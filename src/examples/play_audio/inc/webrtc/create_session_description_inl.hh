#pragma once
#include "play_audio_ext.h"
//
namespace audio {
    //
    class CreateSessionDescriptionObserverImpl : public webrtc::CreateSessionDescriptionObserver {
        OBSERVER_IUNKNOWN()
        OBSERVER_COPY_CONSTRUCT(CreateSessionDescriptionObserverImpl)
        //
        virtual void OnSuccess(
            webrtc::SessionDescriptionInterface* desc
        ) override
        {
            auto inst = (audio::instance_ptr)parent_->Get(OBSERVER_CLSID_Instance);
            std::string sdp;
            desc->ToString(&sdp);
            //
            inst->webrtc_peer_connection->SetLocalDescription(
                (webrtc::SetSessionDescriptionObserver*)
                    parent_->Get(OBSERVER_CLSID_SetSessionDescriptionObserverLocal),
                desc);
            nlohmann::json answer = {{"type", "answer"}, {"sdp", sdp}};
            {
                audio::LOCK lock(audio::mtx_);
                audio::events_websockets_to_webrtc_.push({EVENT_TYPE_ANSWER, answer.dump()});
            }
        }
        virtual void OnFailure(webrtc::RTCError error) override { LOG("%s", error.message()); }
    };
};
