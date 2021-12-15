#include "play_movie_ext.h"
//
struct lws_context* movie::webSocketsStart(
    instance_ptr inst
) {
    const struct lws_protocols protocols[2] = {
        {"websocket", movie::websocketsCallback, 16384, 16384, 0, NULL, 0 },
        LWS_PROTOCOL_LIST_TERM,
    };
    const unsigned LGLVL = (LLL_ERR|LLL_WARN/*|LLL_USER|LLL_NOTICE|LLL_INFO*/|LLL_CLIENT);
    lws_set_log_level(LGLVL, NULL);

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.user = inst;
    inst->context = lws_create_context(&info);

    // Connect if not connected
    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = inst->context;
    ccinfo.address = inst->signaling_server.c_str();
    ccinfo.port = 80;
    ccinfo.path = "/signaling";
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = "websocket";
    ccinfo.local_protocol_name = "websocket";
    ccinfo.alpn = "http/1.1";
    //
    inst->websocket = lws_client_connect_via_info(&ccinfo);
    return(NULL);
}
