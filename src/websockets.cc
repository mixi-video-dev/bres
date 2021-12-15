#include "bres+client_ext.h"

//
struct lws_context*
bres::websockets_init(
    instance_ptr inst
)
{
    const unsigned LGLVL = (LLL_ERR|LLL_WARN|LLL_USER|LLL_NOTICE|LLL_INFO|LLL_CLIENT);
    lws_set_log_level(LGLVL, NULL);
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    //
    inst->protocols[0].name = "websocket";
    inst->protocols[0].callback = bres::websockets_callback;
    inst->protocols[0].per_session_data_size = 16384;
    inst->protocols[0].rx_buffer_size = 16384;
    inst->protocols[0].id = 0;
    inst->protocols[0].user = NULL;
    inst->protocols[0].tx_packet_size = 0;

    inst->protocols[1] = LWS_PROTOCOL_LIST_TERM;

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = inst->protocols;
    info.gid = -1;
    info.uid = -1;
    info.user = inst;
    inst->context = lws_create_context(&info);
    //
    return(inst->context);
}
//
void
bres::websockets_quit(
    instance_ptr inst,
    struct lws_context* context)
{
    lws_context_destroy(context);
}
//
void
bres::websockets_event(
    instance_ptr inst,
    EVENT_TYPE type,
    std::string event
 )
{
    LOG("websockets_event(%u : %u)", type, (unsigned)event.length());
    auto json = nlohmann::json::parse(event);
    //
    switch(type) {
    case EVENT_TYPE_CONNECT:
    case EVENT_TYPE_PONG:
    case EVENT_TYPE_ANSWER:
    case EVENT_TYPE_CANDIDATE:
        inst->sendTarget = event;
        lws_callback_on_writable(inst->websocket);
        break;
    default:
        LOG("not implemented(%u : %u)", type, (unsigned)event.length());
        break;
    }
}

//
int
bres::websockets_callback(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len
)
{
    auto inst = (instance_ptr)lws_context_user(lws_get_context(wsi));
    unsigned char buffer[bres::BUFFLEN + LWS_PRE] = {0};
    //
    switch(reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
        {
            std::string json = "";
            if (inst->order_ == INSTANCE_ORDER_SUBSCRIBE) {
                json = bres::replace(bres::subscribeConnectJson, "$$WEBRTC_CHANNEL_ID$$", inst->channel_id);
            } else {
                json = bres::replace(bres::publishConnectJson, "$$WEBRTC_CHANNEL_ID$$", inst->channel_id);
            }
            json = bres::replace(json, "$$WEBRTC_TOKEN$$", inst->channel_token);
            if (inst->channel_multiple_stream.compare("on") == 0) {
                json = bres::replace(json, "$$WEBRTC_MULTISTREAM$$", "\"multistream\":true,");
            } else {
                json = bres::replace(json, "$$WEBRTC_MULTISTREAM$$", "");
            }
            LOG("%s\n", json.c_str());
            //
            bres::push_event(
                inst,
                EVENT_ORDER_WEBRTC_TO_WEBSOCKETS,
                EVENT_TYPE_CONNECT,
                json
            );
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE:
        {
            memcpy(&inst->recv_buffer[0], in, len);
            inst->recv_written = len;
            std::string res((char*)inst->recv_buffer, inst->recv_written);
            auto json = nlohmann::json::parse(res);
            const std::string type = json["type"];
            //
            if (type == "offer") {
                bres::push_event(
                    inst,
                    EVENT_ORDER_WEBSOCKETS_TO_WEBRTC,
                    EVENT_TYPE_OFFER,
                    res
                );
            } else if (type == "update") {
            } else if (type == "notify") {
            } else if (type == "ping") {
                nlohmann::json json_message = {{"type", "pong"}};
                //
                bres::push_event(
                    inst,
                    EVENT_ORDER_WEBRTC_TO_WEBSOCKETS,
                    EVENT_TYPE_PONG,
                    json_message.dump()
                );
            }
            return(OK);
        }
        case LWS_CALLBACK_CLIENT_WRITEABLE:
        {
            unsigned char* const start = (unsigned char*)(buffer + LWS_PRE);
            unsigned char* const end = (unsigned char*)(buffer + bres::BUFFLEN);
            unsigned char* p = start;
            //
            size_t n = snprintf(
                (char *)p,
                bres::BUFFLEN - 1,
                "%s",
                inst->sendTarget.c_str()
            );
            lws_write(wsi, p, n, LWS_WRITE_TEXT);
            break;
        }
        case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
        {
            lws_client_http_body_pending(wsi, 0);
            break;
        }
        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            LOG("LWS_CALLBACK_CLOSED etc.");
            inst->websocket = NULL;
            break;
        default:
            break;
    }
    return(lws_callback_http_dummy(wsi, reason, user, in, len));
}

//
std::string bres::replace(
      std::string src,
      std::string tgt, 
      std::string dst
)
{
    std::string::size_type  pos(src.find(tgt) );
    while(pos != std::string::npos) {
        src.replace(pos, tgt.length(), dst);
        pos = src.find(tgt, pos + dst.length() );
    }
    return(src);
}