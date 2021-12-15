#pragma once
#include "bres+client_ext.h"

#define OBSERVER_COPY_CONSTRUCT(cls)   \
private:\
    GetObserverInterface* parent_;\
public:\
    cls(\
        GetObserverInterface* parent\
    ): parent_(parent)\
    {}
    
#define OBSERVER_IUNKNOWN() \
public:\
    void AddRef() const override {}\
    rtc::RefCountReleaseStatus Release() const override {\
        return(rtc::RefCountReleaseStatus::kDroppedLastRef);\
    }
//
namespace bres {
    //
    class VideoEncoderFactoryImpl : public webrtc::VideoEncoderFactory {
    public:
        VideoEncoderFactoryImpl() {}
        virtual ~VideoEncoderFactoryImpl() {}
    public:
        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override
        {
            std::vector<webrtc::SdpVideoFormat> supported_codecs;
            supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
            for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs()) {
                supported_codecs.push_back(format);
            }
            //
            return(supported_codecs);
        }
        //
        std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(
            const webrtc::SdpVideoFormat& format
        ) override
        {
            if (std::string(format.name).compare(cricket::kVp8CodecName) == 0) {
                return(webrtc::VP8Encoder::Create());
            }
            if (std::string(format.name).compare(cricket::kVp8CodecName) == 0) {
                return(webrtc::VP9Encoder::Create());
            }
            return(NULL);
        }
    };
    //
    class VideoDecoderFactoryImpl : public webrtc::VideoDecoderFactory {
    public:
        VideoDecoderFactoryImpl() {}
        virtual ~VideoDecoderFactoryImpl() {}
    public:
        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override
        {
            std::vector<webrtc::SdpVideoFormat> supported_codecs;
            supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
            for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs()) {
                supported_codecs.push_back(format);
            }
            return(supported_codecs);
        }
        //
        std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(
            const webrtc::SdpVideoFormat& format
        ) override
        {
            return webrtc::VP8Decoder::Create();
        }
    };
    //
    class SSLVerifier : public rtc::SSLCertificateVerifier {
    public:
        SSLVerifier(){}
        bool Verify(
            const rtc::SSLCertificate& certificate
        ) override
        {
            return true;
        }
    };
    enum OBSERVER_CLSID {
        OBSERVER_CLSID_MIN = 0,
        OBSERVER_CLSID_CreateSessionDescriptionObserver,
        OBSERVER_CLSID_SetSessionDescriptionObserver,
        OBSERVER_CLSID_SetSessionDescriptionObserverLocal,
        OBSERVER_CLSID_Instance,
        OBSERVER_CLSID_MAX
    };
    //
    class GetObserverInterface {
    public:
        virtual void* Get(
            OBSERVER_CLSID cls
        ) = 0;
    };
    //
    class SetRemoteDescriptionObserverInterfaceImpl: public webrtc::SetRemoteDescriptionObserverInterface {
        OBSERVER_IUNKNOWN()
        OBSERVER_COPY_CONSTRUCT(SetRemoteDescriptionObserverInterfaceImpl)
        //
        virtual void OnSetRemoteDescriptionComplete(
            webrtc::RTCError error
        ) override
        {
            LOG("OnSetRemoteDescriptionComplete");
        }
    };
    //
    class SetSessionDescriptionObserverImpl : public webrtc::SetSessionDescriptionObserver {
        OBSERVER_IUNKNOWN()
        OBSERVER_COPY_CONSTRUCT(SetSessionDescriptionObserverImpl)
        //
        virtual void OnSuccess(
        ) override
        {
            LOG("SetSessionDescriptionObserverImpl::OnSuccess");
            auto inst = (bres::instance_ptr)parent_->Get(OBSERVER_CLSID_Instance);
            // init - track
            if (inst->video_track_ && inst->audio_track_) {
                std::string chars = rtc::CreateRandomString(32);
                webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface> >
                    video_add_result = inst->webrtc_peer_connection->AddTrack(inst->video_track_, {chars});
                if (video_add_result.ok()) {
                    rtc::scoped_refptr<webrtc::RtpSenderInterface> video_sender = video_add_result.value();
                    webrtc::RtpParameters parameters = video_sender->GetParameters();
                    parameters.degradation_preference = webrtc::DegradationPreference::BALANCED;
                    //
                    video_sender->SetParameters(parameters);
                } else {
                    PANIC("Cannot add video_track_");
                }
                // audio add track
                auto ret = inst->webrtc_peer_connection->AddTrack(inst->audio_track_, {chars});
                if (!ret.ok()) {
                    PANIC("Cannot add audio_track_(%s)", ret.error().message());
                }
            }
            inst->webrtc_peer_connection->CreateAnswer(
                (webrtc::CreateSessionDescriptionObserver*)
                    parent_->Get(OBSERVER_CLSID_CreateSessionDescriptionObserver),
                webrtc::PeerConnectionInterface::RTCOfferAnswerOptions()
            );
        }
        virtual void OnFailure(
            webrtc::RTCError error
        ) override
        {
            LOG("SetSessionDescriptionObserverImpl::OnFailure");
        }
    };
    //
    class SetSessionDescriptionObserverLocalImpl : public webrtc::SetSessionDescriptionObserver {
        OBSERVER_IUNKNOWN()
        OBSERVER_COPY_CONSTRUCT(SetSessionDescriptionObserverLocalImpl)
        //
        virtual void OnSuccess(
        ) override
        {
            LOG("SetSessionDescriptionObserverLocalImpl::OnSuccess");
            auto inst = (bres::instance_ptr)parent_->Get(OBSERVER_CLSID_Instance);
            inst->webrtc_peer_connection->CreateAnswer(
                (webrtc::CreateSessionDescriptionObserver*)
                    parent_->Get(OBSERVER_CLSID_CreateSessionDescriptionObserver),
                webrtc::PeerConnectionInterface::RTCOfferAnswerOptions()
            );
        }
        virtual void OnFailure(
            webrtc::RTCError error
        ) override
        {
            LOG("SetSessionDescriptionObserverLocalImpl::OnFailure");
        }
    };
    //
    class CreateSessionDescriptionObserverImpl : public webrtc::CreateSessionDescriptionObserver {
        OBSERVER_IUNKNOWN()
        OBSERVER_COPY_CONSTRUCT(CreateSessionDescriptionObserverImpl)
        //
        virtual void OnSuccess(
            webrtc::SessionDescriptionInterface* desc
        ) override
        {
            LOG("CreateSessionDescriptionObserverImpl::OnSuccess");
            auto inst = (bres::instance_ptr)parent_->Get(OBSERVER_CLSID_Instance);
            std::string sdp;
            desc->ToString(&sdp);
            bres::OBSERVER_CLSID clsid = bres::OBSERVER_CLSID_MIN;
            if (inst->order_ == bres::INSTANCE_ORDER_SUBSCRIBE) {
                clsid = bres::OBSERVER_CLSID_SetSessionDescriptionObserver;
            } else {
                clsid = bres::OBSERVER_CLSID_SetSessionDescriptionObserverLocal;
            }
            //
            inst->webrtc_peer_connection->SetLocalDescription(
                (webrtc::SetSessionDescriptionObserver*)
                    parent_->Get(clsid),
                desc);
            nlohmann::json answer = {{"type", "answer"}, {"sdp", sdp}};
            //
            bres::push_event(
                inst,
                EVENT_ORDER_WEBRTC_TO_WEBSOCKETS,
                EVENT_TYPE_ANSWER,
                answer.dump()
            );
        }
        virtual void OnFailure(
            webrtc::RTCError error
        ) override
        {
            LOG("CreateSessionDescriptionObserverImpl::OnFailure(%s)",
                error.message());
        }
    };
    //
    class RTCStatsCollectorCallbackImpl : public webrtc::RTCStatsCollectorCallback {
        OBSERVER_IUNKNOWN()
        OBSERVER_COPY_CONSTRUCT(RTCStatsCollectorCallbackImpl)
        //
        virtual void OnStatsDelivered(
            const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report
        ) override
        {
            LOG("RTCStatsCollectorCallbackImpl::OnStatsDelivered");
        }
    };
    //
    class PeerConnectionObserverImpl:
        public webrtc::PeerConnectionObserver,
        public webrtc::AudioTrackSinkInterface,
        public rtc::VideoSinkInterface<webrtc::VideoFrame>,
        public GetObserverInterface {
    public:
        PeerConnectionObserverImpl(bres::instance_ptr inst) {
            inst_ = inst;
            csd_observ_ = new bres::CreateSessionDescriptionObserverImpl(this);
            ssd_observ_ = new bres::SetSessionDescriptionObserverImpl(this);
            ssd_observ_local_ = new bres::SetSessionDescriptionObserverLocalImpl(this);
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
        // webrtc::AudioTrackSinkInterface interfaces.
        virtual void OnData(
            const void* audio_data,
            int bits_per_sample,
            int sample_rate,
            size_t number_of_channels,
            size_t number_of_frames
        ) override
        {
            // LOG("PeerConnectionObserverImpl::OnData(%d, %d, %d, %d)",
            //     bits_per_sample,
            //     sample_rate,
            //     (int)number_of_channels,
            //     (int)number_of_frames
            // );
            if (!audio_data || bits_per_sample != 16) {
                // not supported audio-format.
                LOG("not supported audio-format(%u)", bits_per_sample);
                return;
            }
            if (sample_rate == 48000 && number_of_frames == 480) {
                std::string pcm((const char*)audio_data,
                    number_of_frames /* num of frame */ * 2 /* 16bit */ * number_of_channels);
                bres::push_event(
                    inst_,
                    bres::EVENT_ORDER_AUDIO,
                    bres::EVENT_TYPE_AUDIO,
                    pcm
                );
            } else if (sample_rate == 16000 && number_of_frames == 160) {
                std::string pcm(
                    number_of_frames /* num of frame */ * 2 /* 16bit */ * number_of_channels * 3,
                    '\0');
                for(auto n = 1;n < (number_of_frames * number_of_channels); n++) {
                    auto o0 = ((short*)audio_data)[n - 1];
                    auto o1 = ((short*)audio_data)[n - 0];
                    // normalize(0-1)
                    auto n0 = ((double)(o0))/32768.0e0;
                    auto n1 = ((double)(o1))/32768.0e0;
                    auto dn = (n1 - n0) / 3.0e0;

                    ((short*)pcm.data())[(n*3) + 0] = (short)(n0 * 32768.0e0);
                    ((short*)pcm.data())[(n*3) + 1] = (short)((n0 + (dn * 1.0e0)) * 32768.0e0);
                    ((short*)pcm.data())[(n*3) + 2] = (short)((n0 + (dn * 2.0e0)) * 32768.0e0);
                    ((short*)pcm.data())[(n*3) + 3] = (short)(n1 * 32768.0e0);
                }
                bres::push_event(
                    inst_,
                    bres::EVENT_ORDER_AUDIO,
                    bres::EVENT_TYPE_AUDIO,
                    pcm
                );
            } else {
                LOG("not supported sampling rate(%u)", sample_rate);
            }
        }
        // rtc::VideoSinkInterface<webrtc::VideoFrame> interfaces.
        virtual void OnFrame(
            const webrtc::VideoFrame& frame
        ) override
        {
            rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer(frame.video_frame_buffer());
            rtc::scoped_refptr<webrtc::I420BufferInterface> i420_buffer = buffer->ToI420();
            static int dbg = 0;
            if ((dbg++ % 900) == 0) {
                LOG("OnFrame(%s) : %8u/ %8u/ %8u",
                    inst_->channel_id.c_str(),
                    (unsigned)frame.width(),
                    (unsigned)i420_buffer->width(),
                    (unsigned)i420_buffer->height()
                );
            }
            char* rgb = NULL;
            if (!bres::pop_bin_event(inst_, EVENT_TYPE_EMPTY_IMAGE, &rgb)) {
                LOG("not enough empty-image buffer(pop_bin_event)[%s]\n", inst_->channel_id.c_str());
                return;
            }
            // ffplay -f rawvideo -video_size 640x360 -pix_fmt rgb32 11_img.bin
            if (i420_buffer->width() != WND_WIDTH || i420_buffer->height() != WND_HEIGHT) {
                rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer = webrtc::I420Buffer::Create(WND_WIDTH, WND_HEIGHT);
                scaled_buffer->ScaleFrom(*i420_buffer);
                //
                libyuv::I420ToABGR(
                    scaled_buffer->DataY(),
                    scaled_buffer->StrideY(),
                    scaled_buffer->DataU(),
                    scaled_buffer->StrideU(),
                    scaled_buffer->DataV(),
                    scaled_buffer->StrideV(),
                    (unsigned char*)rgb,
                    WND_WIDTH * 4,
                    scaled_buffer->width(),
                    scaled_buffer->height());
            } else {
                libyuv::I420ToABGR(
                    i420_buffer->DataY(),
                    i420_buffer->StrideY(),
                    i420_buffer->DataU(),
                    i420_buffer->StrideU(),
                    i420_buffer->DataV(),
                    i420_buffer->StrideV(),
                    (unsigned char*)rgb,
                    i420_buffer->width() * 4,
                    i420_buffer->width(),
                    i420_buffer->height());
            }
            //
            bres::push_bin_event(inst_, EVENT_TYPE_CURRENT_IMAGE, rgb);
        }
        // PeerConnectionObserver interfaces.
        virtual void OnSignalingChange(
            webrtc::PeerConnectionInterface::SignalingState new_state
        ) override
        {
            LOG("OnSignalingChange");
        }
        //
        virtual void OnDataChannel(
            rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel
        ) override
        {
            LOG("OnDataChannel");
        }
        //
        virtual void OnRenegotiationNeeded() override
        {
            LOG("OnRenegotiationNeeded");
        }
        //
        virtual void OnIceConnectionChange(
            webrtc::PeerConnectionInterface::IceConnectionState new_state
        ) override
        {
            LOG("OnIceConnectionChange");
        }
        //
        virtual void OnIceGatheringChange(
            webrtc::PeerConnectionInterface::IceGatheringState new_state
        ) override
        {
            LOG("OnIceGatheringChange");
        }
        //
        virtual void OnIceCandidate(
            const webrtc::IceCandidateInterface* candidate
        ) override
        {
            LOG("OnIceCandidate");
            std::string sdp;
            if (candidate->ToString(&sdp)) {
                nlohmann::json res = {{"type", "candidate"}, {"candidate", sdp}};
                //
                bres::push_event(
                    inst_,
                    EVENT_ORDER_WEBRTC_TO_WEBSOCKETS,
                    EVENT_TYPE_CANDIDATE,
                    res.dump()
                );
            }
        }
        //
        virtual void OnTrack(
            rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver
        ) override
        {
            rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track = transceiver->receiver()->track();
            if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
                LOG("(OnTrack) video");
                ((webrtc::VideoTrackInterface*)track.get())->AddOrUpdateSink(this, rtc::VideoSinkWants());
            } else if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
                LOG("(OnTrack) audio");
                ((webrtc::AudioTrackInterface*)track.get())->AddSink(this);
            }
        }
        //
        virtual void OnRemoveTrack(
            rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver
        ) override
        {
            rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track = receiver->track();
            if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
                LOG("(OnRemoveTrack) video");
            } else if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
                LOG("(OnRemoveTrack) audio");
                ((webrtc::AudioTrackInterface*)track.get())->RemoveSink(this);
            }
        }
    private:
        bres::CreateSessionDescriptionObserverImpl* csd_observ_;
        bres::SetSessionDescriptionObserverImpl* ssd_observ_;
        bres::SetSessionDescriptionObserverLocalImpl* ssd_observ_local_;
        bres::instance_ptr inst_;
    };

}; // namespace scl