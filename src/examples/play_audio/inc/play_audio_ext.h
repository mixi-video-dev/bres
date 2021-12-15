#pragma once
#include "play_audio_def.h"
//
namespace audio {
    // typedef's
    typedef std::lock_guard<std::mutex> LOCK;
    //
    typedef enum EVENT_TYPE_ {
        EVENT_TYPE_NONE = 0,
        EVENT_TYPE_CONNECT,
        EVENT_TYPE_OFFER,
        EVENT_TYPE_ANSWER,
        EVENT_TYPE_CANDIDATE,
        EVENT_TYPE_PONG,
        EVENT_TYPE_MAX
    } EVENT_TYPE;
    //
    typedef struct sevent {
        EVENT_TYPE type;
        std::string event;
    } sevent_t, *sevent_ptr;

    //
    class AudioRender;
    class VideoRender;
    class PeerConnectionObserverImpl;
    typedef struct instance {
        std::string signaling_server = SIGNALING_SERVER;
        std::string channel_id = WEBRTC_CHANNEL_PUBLISH;
        std::string channel_metadata = WEBRTC_METADATA;
        struct lws_context* context = NULL;
        struct lws *websocket = NULL;
        std::string sendTarget;
        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> webrtc_peer_connection_factory;
        rtc::scoped_refptr<webrtc::PeerConnectionInterface> webrtc_peer_connection;
        rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track;
        AudioRender* audioRender;

        PeerConnectionObserverImpl* peer_observer;
        std::unique_ptr<rtc::Thread> message_queue;
        std::unique_ptr<rtc::Thread> worker_thread;
        std::unique_ptr<rtc::Thread> signaling_thread;
    } instance_t, *instance_ptr;

    // function's
    extern int websocketsCallback(
        struct lws *wsi,
        enum lws_callback_reasons reason,
        void *user,
        void *in,
        size_t len
    );
    extern struct lws_context* webSocketsStart(
        instance_ptr inst
    );
    extern int webSocketsStop(
        instance_ptr inst
    );
    extern int webrtcStart(
        instance_ptr inst
    );
    extern int webrtcStop(
        instance_ptr inst
    );
    extern std::string replaceString(
      std::string src,
      std::string tgt, 
      std::string dst
    );
    static const std::string publishConnectJson = std::string("") +
        "{" +
        "\"type\":\"connect\"," +
        "\"role\":\"sendonly\"," +
        "\"channel_id\":\"$$WEBRTC_CHANNEL_ID$$\"," +
        "$$WEBRTC_METADATA$$" +
        "\"audio\":true," +
        "\"video\":false" +
        "}";
    static const int BUFFLEN = 4096;
    static const int OK = 0;

    // instances
    extern instance_t instance_;
    extern bool quit_;
    extern std::vector<std::thread> threads_;
    extern std::mutex mtx_;
    extern std::queue<sevent_t> events_websockets_to_webrtc_;
};

#include "webrtc/observer_interface_inl.hh"
#include "webrtc/audio_render_inl.hh"
#include "webrtc/video_render_inl.hh"
#include "webrtc/create_session_description_inl.hh"
#include "webrtc/set_remote_description_inl.hh"
#include "webrtc/set_session_description_inl.hh"
#include "webrtc/peer_connection_inl.hh"
#include "webrtc/ssl_certificate_verifier_inl.hh"
