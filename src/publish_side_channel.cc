#include "bres+client_ext.h"
//
void
bres::publishSideAddChannel(
    bres::instance_t& inst,
    std::string ev,
    unsigned padding
)
{
    inst.instance_enum = bres::INSTANCE_00;
    inst.quit = false;
    inst.isDraw = 1;
    inst.posX = inst.posY = 0.f;
    inst.scale = 1.f;
    inst.channel_id = WEBRTC_CHANNEL_PUBLISH;
    inst.channel_token = WEBRTC_TOKEN;
    inst.channel_multiple_stream = "";
    inst.order_ = bres::INSTANCE_ORDER_PUBLISH;
    inst.capture_ = new bres::Capture();
    inst.audioRender_ = new AudioRender();
    //
    bres::websockets_init(&inst);
    bres::webrtc_init(&inst);
    bres::event_init(&inst);
    bres::gl_generate_texture(&inst);

    // Connect if not connected
    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = inst.context;
    ccinfo.address = SIGNALING_SERVER;
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
bres::publishSideDeleteChannel(
    bres::instance_t& inst,
    std::string ev,
    unsigned instance_idx
)
{
    LOG("publishSideDeleteChannel(%s)", ev.c_str());
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

    if (inst.capture_) {
        delete inst.capture_;
        inst.capture_ = NULL;
    }
    if (inst.audioRender_) {
        delete inst.audioRender_;
        inst.audioRender_ = NULL;
    }
}

//
void
bres::publishSidePropertyChannel(
    bres::instance_t& inst,
    std::string ev,
    unsigned padding
)
{
    LOG("publishSidePropertyChannel(%s)", ev.c_str());
    auto json = nlohmann::json::parse(ev);

    // set property in specific channel instance
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
}
