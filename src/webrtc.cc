#include "bres+client_ext.h"
#include "bres+client_webrtc_inl.hh"

//
namespace bres {
    static int webrtc_init_counter = 0;
}; // namespace bres
//
void
bres::webrtc_init(
    instance_ptr inst
)
{
    if (webrtc_init_counter == 0) {
        rtc::InitializeSSL();
        webrtc_init_counter ++;
    }
    // start. all threads
    inst->message_queue = rtc::Thread::CreateWithSocketServer();
    RTC_CHECK(inst->message_queue->Start()) << "Failed to start thread";
    inst->worker_thread = rtc::Thread::Create();
    RTC_CHECK(inst->worker_thread->Start()) << "Failed to start thread";
    inst->signaling_thread = rtc::Thread::Create();
    RTC_CHECK(inst->signaling_thread->Start()) << "Failed to start thread";
    //
    webrtc::PeerConnectionFactoryDependencies dependencies;
    dependencies.network_thread = inst->message_queue.get();
    dependencies.worker_thread = inst->worker_thread.get();
    dependencies.signaling_thread = inst->signaling_thread.get();
    dependencies.task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
    dependencies.call_factory = webrtc::CreateCallFactory();
    dependencies.event_log_factory =
        absl::make_unique<webrtc::RtcEventLogFactory>(
            dependencies.task_queue_factory.get());
    // media_dependencies
    cricket::MediaEngineDependencies media_dependencies;
    media_dependencies.task_queue_factory = dependencies.task_queue_factory.get();
    if (inst->order_ == INSTANCE_ORDER_PUBLISH) {
        media_dependencies.adm = inst->worker_thread->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule> >(
            RTC_FROM_HERE, [&] {
                return(inst->audioRender_);
            });
    }
    media_dependencies.audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
    media_dependencies.audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();
    media_dependencies.video_encoder_factory = std::unique_ptr<webrtc::VideoEncoderFactory>(absl::make_unique<bres::VideoEncoderFactoryImpl>());
    media_dependencies.video_decoder_factory = std::unique_ptr<webrtc::VideoDecoderFactory>(absl::make_unique<bres::VideoDecoderFactoryImpl>());
    //
    media_dependencies.audio_mixer = NULL;
    media_dependencies.audio_processing = webrtc::AudioProcessingBuilder().Create();
    media_dependencies.audio_frame_processor = NULL;

    dependencies.media_engine = cricket::CreateMediaEngine(std::move(media_dependencies));
    inst->webrtc_peer_connection_factory = webrtc::CreateModularPeerConnectionFactory(std::move(dependencies));
    if (!inst->webrtc_peer_connection_factory.get()) {
        PANIC("Failed to initialize PeerConnectionFactory");
    }
    webrtc::PeerConnectionFactoryInterface::Options factory_options;
    factory_options.disable_encryption = false;
    factory_options.ssl_max_version = rtc::SSL_PROTOCOL_DTLS_12;
    inst->webrtc_peer_connection_factory->SetOptions(factory_options);
    //
    if (inst->order_ == INSTANCE_ORDER_PUBLISH) {
        std::string chars = rtc::CreateRandomString(32);
        cricket::AudioOptions ao;
        ao.echo_cancellation = false;
        ao.auto_gain_control = false;
        ao.noise_suppression = false;
        ao.highpass_filter = false;
        ao.typing_detection = false;
        ao.residual_echo_detector = false;
        inst->audio_track_ = inst->webrtc_peer_connection_factory->CreateAudioTrack(
                                chars, inst->webrtc_peer_connection_factory->CreateAudioSource(ao));
        if (!inst->audio_track_) {
            PANIC("Cannot create audio_track");
        }
        chars = rtc::CreateRandomString(32);
        rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> video_source =
            webrtc::VideoTrackSourceProxy::Create(
                inst->signaling_thread.get(),
                inst->worker_thread.get(),
                inst->capture_);
        inst->video_track_ = inst->webrtc_peer_connection_factory->CreateVideoTrack(chars, video_source);
        if (inst->video_track_) {
            inst->video_track_->set_content_hint(webrtc::VideoTrackInterface::ContentHint::kText);
            auto vtsrc = inst->video_track_->GetSource();
            if (vtsrc) {
                LOG(": video track getsource.");
            }
        } else {
            PANIC("Cannot create video_track");
        }
    }
    inst->peer_observer = new bres::PeerConnectionObserverImpl(inst);
}
//
void
bres::webrtc_quit(
    instance_ptr inst
)
{
    inst->webrtc_peer_connection_factory = NULL;
    inst->webrtc_peer_connection = NULL;
    inst->message_queue->Stop();
    inst->worker_thread->Stop();
    inst->signaling_thread->Stop();
    inst->message_queue = NULL;
    inst->worker_thread = NULL;
    inst->signaling_thread = NULL;

    delete inst->peer_observer;
}
//
void
bres::webrtc_event(
    instance_ptr inst,
    EVENT_TYPE type,
    std::string event
)
{
    LOG("webrtc_event(%u : %s)", type, event.c_str());
    auto json = nlohmann::json::parse(event);
    //
    if (type == EVENT_TYPE_OFFER) {
        // setup config for create peer connection 
        webrtc::PeerConnectionInterface::RTCConfiguration cfg;
        webrtc::PeerConnectionInterface::IceServers ice_servers;
        //
        auto srvs = json.at("config").at("iceServers");
        for (auto srv : srvs) {
            const std::string username = srv["username"];
            const std::string credential = srv["credential"];
            for (const auto url : srv["urls"]) {
                webrtc::PeerConnectionInterface::IceServer ice_server;
                ice_server.uri = url.get<std::string>();
                ice_server.username = username;
                ice_server.password = credential;
                ice_servers.push_back(ice_server);
            }
        }
        cfg.servers = ice_servers;

        // create peer connection
        cfg.enable_dtls_srtp = true;
        cfg.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
        webrtc::PeerConnectionDependencies dependencies(inst->peer_observer);
        dependencies.tls_cert_verifier = std::unique_ptr<rtc::SSLCertificateVerifier>(new bres::SSLVerifier());
        inst->webrtc_peer_connection = inst->webrtc_peer_connection_factory->CreatePeerConnection(cfg, std::move(dependencies));
        if (!inst->webrtc_peer_connection) {
            PANIC(": CreatePeerConnection failed");
        }
        inst->webrtc_peer_connection->SetAudioPlayout(true);
        inst->webrtc_peer_connection->SetAudioRecording(true);
        //
        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
            webrtc::CreateSessionDescription(
                webrtc::SdpType::kOffer,
                json["sdp"],
                &error);
        if (!session_description) {
            LOG("Failed to create session description: %s : %s",
                error.description.c_str(),
                error.line.c_str());
        } else {
            bres::OBSERVER_CLSID clsid = bres::OBSERVER_CLSID_MIN;
            if (inst->order_ == bres::INSTANCE_ORDER_SUBSCRIBE) {
                clsid = bres::OBSERVER_CLSID_SetSessionDescriptionObserver;
            } else {
                clsid = bres::OBSERVER_CLSID_SetSessionDescriptionObserver;
            }
            inst->webrtc_peer_connection->SetRemoteDescription(
                (webrtc::SetSessionDescriptionObserver*)
                    inst->peer_observer->Get(clsid),
                session_description.release());
        }
    }
}
