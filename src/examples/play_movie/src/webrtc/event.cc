#include "play_movie_ext.h"
//
int movie::webrtcEvent(
    instance_ptr inst
){
    movie::sevent_t ev = {movie::EVENT_TYPE_NONE, ""};
    {
        movie::LOCK lock(movie::mtx_);
        if (!movie::events_websockets_to_webrtc_.empty()) {
            ev = movie::events_websockets_to_webrtc_.front();
            movie::events_websockets_to_webrtc_.pop();
        }
    }
    // event dispatch
    if (ev.type != movie::EVENT_TYPE_NONE) {
        LOG("websockets_event(%u<%u> : %s)", ev.type, (unsigned)ev.event.length(), ev.event.c_str());
        auto json = nlohmann::json::parse(ev.event);
        //
        switch(ev.type) {
        case movie::EVENT_TYPE_CONNECT:
        case movie::EVENT_TYPE_PONG:
        case movie::EVENT_TYPE_ANSWER:
        case movie::EVENT_TYPE_CANDIDATE:
            inst->sendTarget = ev.event;
            lws_callback_on_writable(inst->websocket);
            break;
        case movie::EVENT_TYPE_OFFER:
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
                webrtc::PeerConnectionDependencies dependencies(inst->peer_observer);
                dependencies.tls_cert_verifier = std::unique_ptr<rtc::SSLCertificateVerifier>(new movie::SSLVerifier());
                inst->webrtc_peer_connection = inst->webrtc_peer_connection_factory->CreatePeerConnection(cfg, std::move(dependencies));
                if (!inst->webrtc_peer_connection) {
                    PANIC(": CreatePeerConnection failed");
                }
                inst->webrtc_peer_connection->SetAudioPlayout(false);
                inst->webrtc_peer_connection->SetAudioRecording(false);
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
                    inst->webrtc_peer_connection->SetRemoteDescription(
                        (webrtc::SetSessionDescriptionObserver*)
                            inst->peer_observer->Get(movie::OBSERVER_CLSID_SetSessionDescriptionObserver),
                        session_description.release());
                }
            }
            break;
        default:
            LOG("not implemented(%u : %u)", ev.type, (unsigned)ev.event.length());
            break;
        }
    }
    return(0);
}