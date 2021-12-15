#pragma once
#include "bres+client_def.h"
#include "bres+client_vdevice_inl.hh"

namespace bres {
    // ---------
    // constant
    // ---------
    static const int BUFFLEN = 4096;
    static const std::string subscribeConnectJson = std::string("") +
        "{" +
        "\"type\":\"connect\"," +
        "\"role\":\"recvonly\"," +
        "\"channel_id\":\"$$WEBRTC_CHANNEL_ID$$\"," +
        "\"metadata\":{\"access_token\":\"$$WEBRTC_TOKEN$$\"}," +
        "$$WEBRTC_MULTISTREAM$$" +
        "\"audio\":true," +
        "\"video\":{\"codec_type\":\"VP8\",\"bit_rate\":1000}" +
    "}";
    static const std::string publishConnectJson = std::string("") +
        "{" +
        "\"type\":\"connect\"," +
        "\"role\":\"sendonly\"," +
        "\"channel_id\":\"$$WEBRTC_CHANNEL_ID$$\"," +
        "\"metadata\":{\"access_token\":\"$$WEBRTC_TOKEN$$\"," +
        "\"stats\":false" +
        "}," +
        "$$WEBRTC_MULTISTREAM$$" +
        "\"audio\":true," +
        "\"video\":{\"codec_type\":\"VP8\",\"bit_rate\":1000}" +
    "}";

    static const int OK = 0;
    // ---------
    // typedef
    // ---------
    typedef std::lock_guard<std::mutex> LOCK;
    // ---------
    // enums
    // ---------
    typedef enum EVENT_ORDER_ {
        EVENT_ORDER_NONE = 0,
        EVENT_ORDER_WEBSOCKETS_TO_WEBRTC,
        EVENT_ORDER_WEBRTC_TO_WEBSOCKETS,
        EVENT_ORDER_AUDIO,
        EVENT_ORDER_MAX
    } EVENT_ORDER;
    //
    typedef enum EVENT_TYPE_ {
        EVENT_TYPE_NONE = 0,
        EVENT_TYPE_CONNECT,
        EVENT_TYPE_OFFER,
        EVENT_TYPE_ANSWER,
        EVENT_TYPE_CANDIDATE,
        EVENT_TYPE_PONG,
        EVENT_TYPE_EMPTY_IMAGE,
        EVENT_TYPE_CURRENT_IMAGE,
        EVENT_TYPE_SUBSCRIBE_ADD_CHANNEL,
        EVENT_TYPE_SUBSCRIBE_DELETE_CHANNEL,
        EVENT_TYPE_SUBSCRIBE_PROPERTY_CHANNEL,
        EVENT_TYPE_RESOURCE_ADD_STATIC_IMAGE,
        EVENT_TYPE_RESOURCE_DELETE_STATIC_IMAGE,

        EVENT_TYPE_AUDIO,
        EVENT_TYPE_MAX
    } EVENT_TYPE;
    //
    typedef enum INSTANCE_ {
        INSTANCE_MIN = 0,
        INSTANCE_00, INSTANCE_01, INSTANCE_02, INSTANCE_03,
        INSTANCE_04, INSTANCE_05, INSTANCE_06, INSTANCE_07,
        INSTANCE_MAX,
        INSTANCE_LOCAL_MIN = INSTANCE_MAX + 1,
        INSTANCE_LOCAL_00,
        INSTANCE_LOCAL_MAX
    } INSTANCE;
    //
    typedef enum INSTANCE_ORDER_ {
        INSTANCE_ORDER_MIN = 0,
        INSTANCE_ORDER_SUBSCRIBE = INSTANCE_ORDER_MIN,
        INSTANCE_ORDER_PUBLISH,
        INSTANCE_ORDER_MAX
    } INSTANCE_ORDER;
    //
    typedef enum AUDIO_PARAMETERS_ {
        AUDIO_CHANNEL = 2,
        AUDIO_CHANNELBITSZ = (AUDIO_CHANNEL*(16/8)),
        AUDIO_OUT_CHANNELBITSZ = (AUDIO_CHANNEL*(16/8)),
        AUDIO_RATE48K = 48000,
        AUDIO_PARAMETERS_MAX
    } AUDIO_PARAMETERS;
    
    // ---------
    // instance
    // ---------
    typedef struct sevent {
        EVENT_TYPE type;
        std::string event;
    } sevent_t, *sevent_ptr;
    //
    class PeerConnectionObserverImpl;
    typedef struct instance {
        // websockets
        struct lws_context* context = NULL;
        struct lws *websocket = NULL;
        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> webrtc_peer_connection_factory;
        rtc::scoped_refptr<webrtc::PeerConnectionInterface> webrtc_peer_connection;
        char recv_buffer[BUFFLEN + LWS_PRE] = {0};
        size_t recv_written = 0;
        std::string sendTarget;
        struct lws_protocols protocols[2];
        // webrtc
        std::string channel_id;
        std::string channel_token;
        std::string channel_multiple_stream;
        bres::PeerConnectionObserverImpl* peer_observer;
        std::unique_ptr<rtc::Thread> message_queue;
        std::unique_ptr<rtc::Thread> worker_thread;
        std::unique_ptr<rtc::Thread> signaling_thread;
        INSTANCE_ORDER order_ = INSTANCE_ORDER_MIN;
        // webrtc - publisher
        rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track_;
        rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_;
        bres::Capture* capture_;
        bres::AudioRender* audioRender_;

        // event
        mutable std::mutex mtx_;
        std::queue<bres::sevent_t> events_websockets_to_webrtc_;
        std::queue<bres::sevent_t> events_webrtc_to_websockets_;
        std::queue<bres::sevent_t> events_audio_;
        mutable std::mutex mtx_image_;
        std::queue<char*> events_image_empty;
        std::queue<char*> events_image_current;
        // (audio) - event
        // texture(gl)
        GLuint movieTexture = 0;
        unsigned isDraw = 0;
        float posX = 0.f;
        float posY = 0.f;
        std::string image_path;
        float width = WND_WIDTH;
        float height = WND_HEIGHT;
        unsigned char* init_image = NULL;
        float scale = 1.f;
        float chromakey = 0.f;
        std::vector<char> lastTextureBuffer_;
        // management
        INSTANCE instance_enum;
        unsigned used = 0;
        bool quit = false;
        std::thread event_dispatch_thread_;
        std::thread websockets_thread_;
    } instance_t, *instance_ptr;
    //
    typedef struct events {
        mutable std::mutex mtx_;
        std::queue<bres::sevent_t> events_;
    } events_t, *events_ptr;
    //
    typedef struct client {
        int socket_;
        socklen_t addrlen_;
        struct sockaddr_in addr_;
        struct event_base *base_;
        struct event *cli_;
        std::string req_;
    } client_t, *client_ptr;


    // ---------
    // functions / websockets
    // ---------
    extern int
    websockets_callback(
        struct lws *wsi,
        enum lws_callback_reasons reason,
        void *user,
        void *in,
        size_t len
    );
    extern struct lws_context*
    websockets_init(
        instance_ptr inst
    );
    extern void
    websockets_quit(
        instance_ptr inst,
        struct lws_context* context
    );
    extern void
    websockets_event(
        instance_ptr inst,
        EVENT_TYPE type,
        std::string event
    );

    // ---------
    // functions / webrtc
    // ---------
    extern void webrtc_init(
        instance_ptr inst
    );
    extern void webrtc_quit(
        instance_ptr inst
    );
    extern void
    webrtc_event(
        instance_ptr inst,
        EVENT_TYPE type,
        std::string event
    );

    // ---------
    // functions / messaging fifo
    // ---------
    extern void event_init(
        instance_ptr inst
    );
    extern void event_quit(
        instance_ptr inst
    );
    extern void
    push_event(
        instance_ptr inst,
        EVENT_ORDER order,
        EVENT_TYPE type,
        std::string event
    );
    extern bool
    pop_event(
        instance_ptr inst,
        EVENT_ORDER order,
        EVENT_TYPE& type,
        std::string& event
    );
    extern void
    push_bin_event(
        instance_ptr inst,
        EVENT_TYPE type,
        char* paddr
    );
    extern bool
    pop_bin_event(
        instance_ptr inst,
        EVENT_TYPE type,
        char** ppaddr
    );
    // ---------
    // functions / opengl
    // ---------
    extern GLFWwindow* gl_init(void);
    extern void
    gl_quit(
        instance_ptr inst
    );
    extern void
    gl_generate_texture(
        instance_ptr inst
    );
    extern void
    gl_movie_play(
        instance_ptr inst,
        char* frame,
        float alpha,
        float toblack,
        float x,
        float y,
        float w,
        float h,
        float scalex,
        float scaley
    );
    extern void
    calculateVec(
        GLfloat x,
        GLfloat y,
        GLfloat w,
        GLfloat h,
        GLfloat box[6][4]
    );
    //
    extern std::string
    replace(
        std::string src,
        std::string tgt,
        std::string dst
    );
    //
    extern int
    gl_generateShader(
        const char* vShaderCode,
        const char* fShaderCode
    );
    // ---------
    // functions / telnet server
    // ---------
    extern void
    telnet_service(
        void* arg
    );
    // ---------
    // functions / subscribe side channel
    // ---------
    extern void
    subscribeSideAddChannel(
        std::vector<bres::instance_t>& pooled_instances,
        std::string ev,
        unsigned padding
    );
    //
    extern void
    subscribeSideDeleteChannel(
        std::vector<bres::instance_t>& pooled_instances,
        std::string ev,
        unsigned padding
    );
    //
    extern void
    subscribeSidePropertyChannel(
        std::vector<bres::instance_t>& pooled_instances,
        std::string ev,
        unsigned padding
    );
    // ---------
    // functions / publish side channel
    // ---------
    extern void
    publishSideAddChannel(
        bres::instance_t& inst,
        std::string ev,
        unsigned padding
    );
    //
    extern void
    publishSideDeleteChannel(
        bres::instance_t& inst,
        std::string ev,
        unsigned padding
    );
    //
    extern void
    publishSidePropertyChannel(
        bres::instance_t& inst,
        std::string ev,
        unsigned padding
    );
    // ---------
    // functions / audio
    // ---------
    extern void
    audioInit(
        void* arg
    );
    //
    extern void
    audioCallback(
        void* arg,
        unsigned char* data,
        int len
    );
    // ---------
    // functions / blackmagic pipeline
    // ---------
    extern void
    bm_pipeline_service(
        void* arg
    );
    //
    extern void
    bm_add_pipeline_frame(
        const char* frame,
        unsigned framelen
    );
    // ---------
    // functions / resource operations
    // ---------
    extern void
    resourceAddStaticImage(
        std::vector<bres::instance_t>& pooled_instances,
        std::string ev,
        unsigned padding
    );
    //
    extern void
    resourceDeleteStaticImage(
        std::vector<bres::instance_t>& pooled_instances,
        std::string ev,
        unsigned padding
    );

    // ---------
    // functions / setAffinity
    // ---------
    extern void
    setAffinity(
        int cpu
    );

    // global instances
    extern bool quit;

}; // namespace bres

