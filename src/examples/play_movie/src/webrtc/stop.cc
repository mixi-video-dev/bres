#include "play_movie_ext.h"
//
int movie::webrtcStop(
    instance_ptr inst
){
    inst->webrtc_peer_connection_factory = NULL;
    inst->webrtc_peer_connection = NULL;
    inst->message_queue->Stop();
    inst->worker_thread->Stop();
    inst->signaling_thread->Stop();
    inst->message_queue = NULL;
    inst->worker_thread = NULL;
    inst->signaling_thread = NULL;

    delete inst->peer_observer;
    delete inst->videoRender;
    inst->peer_observer = NULL;
    inst->videoRender = NULL;
    lws_context_destroy(inst->context);

    return(OK);
}
