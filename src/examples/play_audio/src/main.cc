#include "play_audio_ext.h"
/*
export SIGNALING_SERVER=xxxx
export WEBRTC_CHANNEL_PUBLISH=xxxx
export WEBRTC_TOKEN=xxxx
export AUDIO_PCM=xxxxx
*/

// global extern , instanciate.
audio::instance_t audio::instance_;
bool audio::quit_ = false;
std::vector<std::thread> audio::threads_;
std::mutex audio::mtx_;
std::queue<audio::sevent_t> audio::events_websockets_to_webrtc_;

// Entry Point
int main(
    int argc,
    char *argv[]
)
{
    struct timeval tm;
    unsigned long prev_usec = 0;
    std::string audio_wav = AUDIO_WAV;
    std::vector<int16_t> audio_buf;

    openlog(argv[0], LOG_PERROR|LOG_PID,LOG_LOCAL2);
    srand((unsigned) time(NULL) * getpid());
    // target audio
    FILE* fp = fopen(audio_wav.c_str(), "rb");
    if (!fp) {
        PANIC("not found(audio file %s)", audio_wav.c_str());
    }
    fread(audio_buf.data(), audio_buf.size() * sizeof(int16_t), 1, fp);

    //
    audio::instance_.audioRender = new audio::AudioRender();
    //
    audio::webrtcStart(&audio::instance_);
    audio::webSocketsStart(&audio::instance_);
    // websockets loop
    audio::threads_.push_back(std::thread(
        [&]{
            while(!audio::quit_) {
                lws_service(audio::instance_.context, 250);
            }
        }));
    //
    while (!audio::quit_){
        gettimeofday(&tm, NULL);
        auto now = tm.tv_sec*(uint64_t)1000000+tm.tv_usec;
        if (((now - prev_usec) >= 10000.0 /*  10ms */)) {
            audio::sevent_t ev = {audio::EVENT_TYPE_NONE, ""};
            {
                audio::LOCK lock(audio::mtx_);
                if (!audio::events_websockets_to_webrtc_.empty()) {
                    ev = audio::events_websockets_to_webrtc_.front();
                    audio::events_websockets_to_webrtc_.pop();
                }
            }
            // event dispatch
            if (ev.type != audio::EVENT_TYPE_NONE) {
                LOG("websockets_event(%u<%u> : %s)", ev.type, (unsigned)ev.event.length(), ev.event.c_str());
                auto json = nlohmann::json::parse(ev.event);
                //
                switch(ev.type) {
                case audio::EVENT_TYPE_CONNECT:
                case audio::EVENT_TYPE_PONG:
                case audio::EVENT_TYPE_ANSWER:
                case audio::EVENT_TYPE_CANDIDATE:
                    audio::instance_.sendTarget = ev.event;
                    lws_callback_on_writable(audio::instance_.websocket);
                    break;
                case audio::EVENT_TYPE_OFFER:
                    {
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
                        webrtc::PeerConnectionDependencies dependencies(audio::instance_.peer_observer);
                        dependencies.tls_cert_verifier = std::unique_ptr<rtc::SSLCertificateVerifier>(new audio::SSLVerifier());
                        audio::instance_.webrtc_peer_connection = audio::instance_.webrtc_peer_connection_factory->CreatePeerConnection(cfg, std::move(dependencies));
                        if (!audio::instance_.webrtc_peer_connection) {
                            PANIC(": CreatePeerConnection failed");
                        }
                        audio::instance_.webrtc_peer_connection->SetAudioPlayout(true);
                        audio::instance_.webrtc_peer_connection->SetAudioRecording(true);
                        //
                        webrtc::SdpParseError error;
                        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
                            webrtc::CreateSessionDescription(
                                webrtc::SdpType::kOffer,
                                json["sdp"],
                                &error);
                        if (!session_description) {
                            PANIC("Failed to create session description: %s : %s",
                                error.description.c_str(),
                                error.line.c_str());
                        } else {
                            audio::instance_.webrtc_peer_connection->SetRemoteDescription(
                                (webrtc::SetSessionDescriptionObserver*)
                                    audio::instance_.peer_observer->Get(audio::OBSERVER_CLSID_SetSessionDescriptionObserver),
                                session_description.release());
                        }
                    }
                    break;
                default:
                    LOG("not implemented(%u : %u)", ev.type, (unsigned)ev.event.length());
                    break;
                }
            }
            // audio play
            // every 10 msec => 48K,sampling = 480 x 2 channel.
            audio_buf.resize(480 * 2);
            auto ret = fread(audio_buf.data(), audio_buf.size()*sizeof(int16_t), 1, fp);
            if (ret != 1) {
                LOG("audio EOF.(%d , %d)", (int)ret, (int)audio_buf.size());
                break;
            } else {
                audio::instance_.audioRender->Render(audio_buf);
            }
            prev_usec = now;
        }
    }
    audio::quit_ = true;
    for(auto &th : audio::threads_) {
        th.join();
    }
    audio::webrtcStop(&audio::instance_);
    audio::webSocketsStop(&audio::instance_);
    //
    closelog();
    fclose(fp);
    return(0);
}
