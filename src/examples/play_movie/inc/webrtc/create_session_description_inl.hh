#pragma once
#include "play_movie_ext.h"
//
namespace movie {
    //
    class CreateSessionDescriptionObserverImpl : public webrtc::CreateSessionDescriptionObserver {
        OBSERVER_IUNKNOWN()
        OBSERVER_COPY_CONSTRUCT(CreateSessionDescriptionObserverImpl)
        //
        virtual void OnSuccess(
            webrtc::SessionDescriptionInterface* desc
        ) override
        {
            auto inst = (movie::instance_ptr)parent_->Get(OBSERVER_CLSID_Instance);
            std::string sdp;
            desc->ToString(&sdp);
            //
            inst->webrtc_peer_connection->SetLocalDescription(
                (webrtc::SetSessionDescriptionObserver*)
                    parent_->Get(OBSERVER_CLSID_SetSessionDescriptionObserverLocal),
                desc);
            nlohmann::json answer = {{"type", "answer"}, {"sdp", sdp}};
            {
                movie::LOCK lock(movie::mtx_);
                movie::events_websockets_to_webrtc_.push({EVENT_TYPE_ANSWER, answer.dump()});
            }
        }
        virtual void OnFailure(webrtc::RTCError error) override { LOG("%s", error.message()); }
    };
};
