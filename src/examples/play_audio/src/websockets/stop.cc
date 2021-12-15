#include "play_audio_ext.h"
//
int audio::webSocketsStop(
    instance_ptr inst
) {
    lws_context_destroy(inst->context);
    return(OK);
}
