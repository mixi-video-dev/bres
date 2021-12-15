#include "play_movie_ext.h"
//
int movie::webrtcStart(
    instance_ptr inst
){
    rtc::InitializeSSL();
    inst->message_queue = rtc::Thread::CreateWithSocketServer(); inst->message_queue->Start();
    inst->worker_thread = rtc::Thread::Create(); inst->worker_thread->Start();
    inst->signaling_thread = rtc::Thread::Create(); inst->signaling_thread->Start();

    webrtc::PeerConnectionFactoryDependencies dependencies;
    dependencies.network_thread = inst->message_queue.get();
    dependencies.worker_thread = inst->worker_thread.get();
    dependencies.signaling_thread = inst->signaling_thread.get();
    dependencies.task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
    dependencies.call_factory = webrtc::CreateCallFactory();
    dependencies.event_log_factory = absl::make_unique<webrtc::RtcEventLogFactory>(dependencies.task_queue_factory.get());
    // media_dependencies
    cricket::MediaEngineDependencies media_dependencies;
    media_dependencies.task_queue_factory = dependencies.task_queue_factory.get();
    media_dependencies.adm = inst->worker_thread->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule> >(
            RTC_FROM_HERE, [&] {
                return(inst->audioRender);
            });
    media_dependencies.audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
    media_dependencies.audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();
    media_dependencies.video_encoder_factory = std::unique_ptr<webrtc::VideoEncoderFactory>(absl::make_unique<movie::VideoEncoderFactoryImpl>());
    media_dependencies.video_decoder_factory = std::unique_ptr<webrtc::VideoDecoderFactory>(absl::make_unique<movie::VideoDecoderFactoryImpl>());
    media_dependencies.audio_mixer = NULL;
    media_dependencies.audio_processing = webrtc::AudioProcessingBuilder().Create();
    media_dependencies.audio_frame_processor = NULL;
    //
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
    std::string chars;
    rtc::CreateRandomString(32, &chars);
    cricket::AudioOptions ao;
    ao.echo_cancellation = false;
    ao.auto_gain_control = false;
    ao.noise_suppression = false;
    ao.highpass_filter = false;
    ao.typing_detection = false;
    ao.residual_echo_detector = false;
    inst->audio_track = inst->webrtc_peer_connection_factory->CreateAudioTrack(
                            chars, inst->webrtc_peer_connection_factory->CreateAudioSource(ao));
    if (!inst->audio_track) {
        PANIC("Cannot create audio_track");
    }
    // video track
    rtc::CreateRandomString(32, &chars);
    rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> video_source =
        webrtc::VideoTrackSourceProxy::Create(
            inst->signaling_thread.get(),
            inst->worker_thread.get(),
            inst->videoRender);
    inst->video_track = inst->webrtc_peer_connection_factory->CreateVideoTrack(chars, video_source);
    if (inst->video_track) {
        inst->video_track->set_content_hint(webrtc::VideoTrackInterface::ContentHint::kText);
        auto vtsrc = inst->video_track->GetSource();
        if (vtsrc) {
            LOG(": video track getsource.");
        }
    } else {
        PANIC("Cannot create video_track");
    }

    inst->peer_observer = new movie::PeerConnectionObserverImpl(&movie::instance_);

    return(OK);
}
