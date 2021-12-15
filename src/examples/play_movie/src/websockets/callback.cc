#include "play_movie_ext.h"

int movie::websocketsCallback(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len
)
{
    auto inst = (instance_ptr)lws_context_user(lws_get_context(wsi));
    unsigned char buffer[movie::BUFFLEN + LWS_PRE] = {0};
    //
    switch(reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
        {
            auto json = movie::replaceString(movie::publishConnectJson, "$$WEBRTC_CHANNEL_ID$$", inst->channel_id);
            json = movie::replaceString(json, "$$WEBRTC_METADATA$$", inst->channel_metadata);
            {
                movie::LOCK lock(movie::mtx_);
                movie::events_websockets_to_webrtc_.push({movie::EVENT_TYPE_CONNECT, json});
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE:
        {
            std::string res((char*)in, len);
            auto json = nlohmann::json::parse(res);
            const std::string type = json["type"];
            //
            if (type == "offer") {
                movie::LOCK lock(mtx_);
                movie::events_websockets_to_webrtc_.push({ movie::EVENT_TYPE_OFFER, res});
            } else if (type == "update") {
            } else if (type == "notify") {
            } else if (type == "ping") {
                movie::LOCK lock(mtx_);
                nlohmann::json json_message = {{"type", "pong"}};
                movie::events_websockets_to_webrtc_.push({ movie::EVENT_TYPE_PONG, json_message.dump()});
            }
            return(OK);
        }
        case LWS_CALLBACK_CLIENT_WRITEABLE:
        {
            unsigned char* const start = (unsigned char*)(buffer + LWS_PRE);
            unsigned char* const end = (unsigned char*)(buffer + movie::BUFFLEN);
            unsigned char* p = start;
            //
            size_t n = snprintf(
                (char *)p,
                movie::BUFFLEN - 1,
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
