#pragma once
#include "play_movie_ext.h"
//
namespace movie {
    //
    class SetRemoteDescriptionObserverInterfaceImpl: public webrtc::SetRemoteDescriptionObserverInterface {
        OBSERVER_IUNKNOWN()
        OBSERVER_COPY_CONSTRUCT(SetRemoteDescriptionObserverInterfaceImpl)
        //
        virtual void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override { LOG("%s", error.message()); }
    };
};
