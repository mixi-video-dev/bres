#pragma once
#include "play_audio_ext.h"
//
namespace audio {
    //
    class SetSessionDescriptionObserverImpl : public webrtc::SetSessionDescriptionObserver {
        OBSERVER_IUNKNOWN()
        OBSERVER_COPY_CONSTRUCT(SetSessionDescriptionObserverImpl)
        //
        virtual void OnSuccess(
        ) override
        {
            auto inst = (audio::instance_ptr)parent_->Get(OBSERVER_CLSID_Instance);
            std::string chars;
            rtc::CreateRandomString(32, &chars);
            // audio add track
            auto ret = inst->webrtc_peer_connection->AddTrack(inst->audio_track, {chars});
            if (!ret.ok()) {
                PANIC("Cannot add audio_track_(%s)", ret.error().message());
            }
            inst->webrtc_peer_connection->CreateAnswer(
                (webrtc::CreateSessionDescriptionObserver*)
                    parent_->Get(OBSERVER_CLSID_CreateSessionDescriptionObserver),
                webrtc::PeerConnectionInterface::RTCOfferAnswerOptions()
            );
        }
        virtual void OnFailure(webrtc::RTCError error) override { LOG("%s", error.message()); }
    };
    //
    class SetSessionDescriptionObserverLocalImpl : public webrtc::SetSessionDescriptionObserver {
        OBSERVER_IUNKNOWN()
        OBSERVER_COPY_CONSTRUCT(SetSessionDescriptionObserverLocalImpl)
        //
        virtual void OnSuccess(
        ) override
        {
            auto inst = (audio::instance_ptr)parent_->Get(OBSERVER_CLSID_Instance);
            inst->webrtc_peer_connection->CreateAnswer(
                (webrtc::CreateSessionDescriptionObserver*)
                    parent_->Get(OBSERVER_CLSID_CreateSessionDescriptionObserver),
                webrtc::PeerConnectionInterface::RTCOfferAnswerOptions()
            );
        }
        virtual void OnFailure(webrtc::RTCError error) override { LOG("%s", error.message()); }
    };
};