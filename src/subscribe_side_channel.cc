#include "bres+client_ext.h"
//
void
bres::subscribeSideAddChannel(
    std::vector<bres::instance_t>& pooled_instances,
    std::string ev,
    unsigned padding
)
{
    unsigned instance_idx = bres::INSTANCE_MIN;
    //
    for(auto& inst : pooled_instances) {
        if (inst.instance_enum == bres::INSTANCE_MIN) {
            break;
        }
        instance_idx ++;
    }
    auto json = nlohmann::json::parse(ev);
    if (json.find("channel_id") == json.end() ||
        json.find("channel_token") == json.end() ||
        json.find("channel_multiple_stream") == json.end() ||
        json.find("signaling_server") == json.end() ||
        instance_idx >= bres::INSTANCE_MAX) {
        return;
    }
    auto& inst = pooled_instances.at(instance_idx);
    inst.instance_enum = bres::INSTANCE_00;
    inst.quit = false;
    inst.isDraw = 0;
    inst.posX = inst.posY = 0.f;
    inst.scale = 1.f;
    inst.channel_id = json["channel_id"].get<std::string>();
    inst.channel_token = json["channel_token"].get<std::string>();
    inst.channel_multiple_stream = json["channel_multiple_stream"].get<std::string>();
    //
    bres::websockets_init(&inst);
    bres::webrtc_init(&inst);
    bres::event_init(&inst);
    bres::gl_generate_texture(&inst);

    // Connect if not connected
    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = inst.context;
    ccinfo.address = json["signaling_server"].get<std::string>().c_str();
    ccinfo.port = 80;
    ccinfo.path = "/signaling";
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = "websocket";
    ccinfo.local_protocol_name = "websocket";
    ccinfo.alpn = "http/1.1";
    //
    inst.websocket = lws_client_connect_via_info(&ccinfo);
    // event dispatch
    inst.event_dispatch_thread_ = std::thread(
        [&inst]{
            while(!inst.quit) {
                usleep(100);
                bres::EVENT_TYPE type;
                std::string event;
                // websockets -> webrtc
                if (bres::pop_event(
                    &inst,
                    bres::EVENT_ORDER_WEBSOCKETS_TO_WEBRTC,
                    type,
                    event))
                {
                    bres::webrtc_event(&inst, type, event);
                }
                if (bres::pop_event(
                    &inst,
                    bres::EVENT_ORDER_WEBRTC_TO_WEBSOCKETS,
                    type,
                    event))
                {
                    bres::websockets_event(&inst, type, event);
                }
            }
        });
    // websockets loop
    inst.websockets_thread_ = std::thread(
        [&inst]{
            while(!inst.quit) {
                lws_service(inst.context, 250);
            }
        });
}
//
void
bres::subscribeSideDeleteChannel(
    std::vector<bres::instance_t>& pooled_instances,
    std::string ev,
    unsigned instance_idx
)
{
    LOG("subscribeSideDeleteChannel(%s)", ev.c_str());
    auto json = nlohmann::json::parse(ev);
    if (json.find("channel_id") == json.end()) {
        return;
    }
    auto channel_id = json["channel_id"].get<std::string>();

    for(auto& inst : pooled_instances) {
        // チャネル指定で停止
        if (!inst.channel_id.compare(channel_id)) {
            inst.quit = true;
            inst.isDraw = 0;
            inst.instance_enum = bres::INSTANCE_MIN;
            usleep(MAX(100, 250000) << 1);
            //
            inst.event_dispatch_thread_.join();
            inst.websockets_thread_.join();
            //
            bres::websockets_quit(&inst, inst.context);
            bres::webrtc_quit(&inst);
            bres::gl_quit(&inst);
        }
    }
}

//
void
bres::subscribeSidePropertyChannel(
    std::vector<bres::instance_t>& pooled_instances,
    std::string ev,
    unsigned padding
)
{
    LOG("subscribeSidePropertyChannel(%s)", ev.c_str());

    auto json = nlohmann::json::parse(ev);
    if (json.find("channel_id") == json.end()) {
        return;
    }
    auto channel_id = json["channel_id"].get<std::string>();

    for(auto& inst : pooled_instances) {
        // set property in specific channel instance
        if (!inst.channel_id.compare(channel_id)) {
            if (json.find("is_draw") != json.end()) {
                inst.isDraw = (unsigned)std::atoi(json["is_draw"].get<std::string>().c_str());
            }
            if (json.find("pos_x") != json.end()) {
                inst.posX = std::atof(json["pos_x"].get<std::string>().c_str());
            }
            if (json.find("pos_y") != json.end()) {
                inst.posY = std::atof(json["pos_y"].get<std::string>().c_str());
            }
            if (json.find("scale") != json.end()) {
                inst.scale = std::atof(json["scale"].get<std::string>().c_str());
            }
            if (json.find("chromakey") != json.end()) {
                inst.chromakey = std::atof(json["chromakey"].get<std::string>().c_str());
            }
            break;
        }
    }
}
