#pragma once
#include "play_movie_ext.h"
//
namespace movie {
    //
    class PeerConnectionObserverImpl:
        public webrtc::PeerConnectionObserver,
        public GetObserverInterface {
    public:
        PeerConnectionObserverImpl(movie::instance_ptr inst) {
            inst_ = inst;
            csd_observ_ = new movie::CreateSessionDescriptionObserverImpl(this);
            ssd_observ_ = new movie::SetSessionDescriptionObserverImpl(this);
            ssd_observ_local_ = new movie::SetSessionDescriptionObserverLocalImpl(this);
        }
        virtual ~PeerConnectionObserverImpl() {
            inst_ = NULL;
            delete csd_observ_;
            delete ssd_observ_;
            delete ssd_observ_local_;
        }
    public:
        virtual void* Get(
            OBSERVER_CLSID cls
        ) override
        {
            switch(cls) {
            case OBSERVER_CLSID_CreateSessionDescriptionObserver:
                return(csd_observ_);
            case OBSERVER_CLSID_SetSessionDescriptionObserver:
                return(ssd_observ_);
            case OBSERVER_CLSID_SetSessionDescriptionObserverLocal:
                return(ssd_observ_local_);
            case OBSERVER_CLSID_Instance:
                return(inst_);
            default:
                PANIC("unknown clsid");
                break;
            }
            return(NULL);
        }
    public:
        // PeerConnectionObserver interfaces.
        virtual void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override { LOG("OnSignalingChange"); }
        virtual void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override { LOG("OnDataChannel"); }
        virtual void OnRenegotiationNeeded() override { LOG("OnRenegotiationNeeded"); }
        virtual void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override { LOG("OnIceConnectionChange"); }
        virtual void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override { LOG("OnIceGatheringChange"); }
        //
        virtual void OnIceCandidate(
            const webrtc::IceCandidateInterface* candidate
        ) override
        {
            std::string sdp;
            if (candidate->ToString(&sdp)) {
                nlohmann::json json = {{"type", "candidate"}, {"candidate", sdp}};
                {
                    movie::LOCK lock(movie::mtx_);
                    movie::events_websockets_to_webrtc_.push({EVENT_TYPE_CANDIDATE, json.dump()});
                }
            }
        }
    private:
        movie::CreateSessionDescriptionObserverImpl* csd_observ_;
        movie::SetSessionDescriptionObserverImpl* ssd_observ_;
        movie::SetSessionDescriptionObserverLocalImpl* ssd_observ_local_;
        movie::instance_ptr inst_;
    };
};