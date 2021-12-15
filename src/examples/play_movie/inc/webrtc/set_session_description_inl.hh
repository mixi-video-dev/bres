#pragma once
#include "play_movie_ext.h"
//
namespace movie {
    //
    class SetSessionDescriptionObserverImpl : public webrtc::SetSessionDescriptionObserver {
        OBSERVER_IUNKNOWN()
        OBSERVER_COPY_CONSTRUCT(SetSessionDescriptionObserverImpl)
        //
        virtual void OnSuccess(
        ) override
        {
            auto inst = (movie::instance_ptr)parent_->Get(OBSERVER_CLSID_Instance);
            std::string chars;
            rtc::CreateRandomString(32, &chars);
            webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface> >
                video_add_result = inst->webrtc_peer_connection->AddTrack(inst->video_track, {chars});
            if (video_add_result.ok()) {
                rtc::scoped_refptr<webrtc::RtpSenderInterface> video_sender = video_add_result.value();
                webrtc::RtpParameters parameters = video_sender->GetParameters();
                parameters.degradation_preference = webrtc::DegradationPreference::BALANCED;
                //
                video_sender->SetParameters(parameters);
            } else {
                PANIC("Cannot add video_track_");
            }
            LOG("SetSessionDescriptionObserverImpl::OnSuccess");
            //
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
            auto inst = (movie::instance_ptr)parent_->Get(OBSERVER_CLSID_Instance);
            inst->webrtc_peer_connection->CreateAnswer(
                (webrtc::CreateSessionDescriptionObserver*)
                    parent_->Get(OBSERVER_CLSID_CreateSessionDescriptionObserver),
                webrtc::PeerConnectionInterface::RTCOfferAnswerOptions()
            );
        }
        virtual void OnFailure(webrtc::RTCError error) override { LOG("%s", error.message()); }
    };
};