#include "play_movie_ext.h"
//
int movie::webSocketsStop(
    instance_ptr inst
) {
    lws_context_destroy(inst->context);
    return(OK);
}
