#include "bres+client_ext.h"

//
void
bres::push_event(
    instance_ptr inst,
    EVENT_ORDER order,
    EVENT_TYPE type,
    std::string event
)
{
    bres::LOCK lock(inst->mtx_);
    if (order == EVENT_ORDER_WEBSOCKETS_TO_WEBRTC) {
        inst->events_websockets_to_webrtc_.push({type, event});
    } else if (order == EVENT_ORDER_WEBRTC_TO_WEBSOCKETS) {
        inst->events_webrtc_to_websockets_.push({type, event});
    } else if (order == EVENT_ORDER_AUDIO) {
        inst->events_audio_.push({type, event});
    }
}
//
bool
bres::pop_event(
    instance_ptr inst,
    EVENT_ORDER order,
    EVENT_TYPE& type,
    std::string& event
)
{
    bres::LOCK lock(inst->mtx_);
    if (order == EVENT_ORDER_WEBSOCKETS_TO_WEBRTC) {
        if (!inst->events_websockets_to_webrtc_.empty()) {
            auto e = inst->events_websockets_to_webrtc_.front();
            inst->events_websockets_to_webrtc_.pop();
            type = e.type;
            event = e.event;
            return(true);
        }
    } else if (order == EVENT_ORDER_WEBRTC_TO_WEBSOCKETS) {
        if (!inst->events_webrtc_to_websockets_.empty()) {
            auto e = inst->events_webrtc_to_websockets_.front();
            inst->events_webrtc_to_websockets_.pop();
            type = e.type;
            event = e.event;
            return(true);
        }
    } else if (order == EVENT_ORDER_AUDIO) {
        if (!inst->events_audio_.empty()) {
            auto e = inst->events_audio_.front();
            inst->events_audio_.pop();
            type = e.type;
            event = e.event;
            return(true);
        }
    }
    return(false);
}
//
void
bres::event_init(
    instance_ptr inst
)
{
    bres::LOCK lock(inst->mtx_image_);
    for(auto n = 0;n < 30;n++) {
        auto bf = (char*)malloc(WND_WIDTH*WND_HEIGHT*4);
        bzero(bf,WND_WIDTH*WND_HEIGHT*4);
        inst->events_image_empty.push(bf);
    }
}
void
bres::event_quit(
    instance_ptr inst
)
{
    bres::LOCK lock(inst->mtx_image_);
    while (!inst->events_image_empty.empty()) {
        auto e = inst->events_image_empty.front();
        free(e);
        inst->events_image_empty.pop();
    }    
    std::queue<char*> empty;
    std::swap(inst->events_image_empty, empty);
}

//
void
bres::push_bin_event(
    instance_ptr inst,
    EVENT_TYPE type,
    char* paddr
)
{
    bres::LOCK lock(inst->mtx_image_);
    if (type == EVENT_TYPE_EMPTY_IMAGE) {
        inst->events_image_empty.push(paddr);
    } else if (type == EVENT_TYPE_CURRENT_IMAGE) {
        inst->events_image_current.push(paddr);
    }
}
//
bool
bres::pop_bin_event(
    instance_ptr inst,
    EVENT_TYPE type,
    char** ppaddr
)
{
    bres::LOCK lock(inst->mtx_image_);
    if (type == EVENT_TYPE_EMPTY_IMAGE) {
        if (!inst->events_image_empty.empty()) {
            auto e = inst->events_image_empty.front();
            inst->events_image_empty.pop();
            (*ppaddr) = e;
            return(true);
        }
    } else if (type == EVENT_TYPE_CURRENT_IMAGE) {
        if (!inst->events_image_current.empty()) {
            auto e = inst->events_image_current.front();
            inst->events_image_current.pop();
            (*ppaddr) = e;
            return(true);
        }
    }
    return(false);
}
