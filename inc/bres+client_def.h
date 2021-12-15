#pragma once

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "memory.h"
#include "fcntl.h"
#include "stdarg.h"
#include "sys/time.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/mman.h"
#include "signal.h"
#include "syslog.h"

#define _USE_MATH_DEFINES

#include "cmath"
#include "fstream"
#include "sstream"
#include "cassert"
#include "string"
#include "iostream"
#include "vector"
#include "map"
#include "queue"
#include "deque"
#include "codecvt"
#include "locale"
#include "functional"
#include "tuple"
#include "mutex"
#include "thread"
#include "bitset"
#include "typeinfo"
#include "experimental/filesystem"
#include "arpa/inet.h"
#include "cxxabi.h"

// websocket
#include <libwebsockets.h>

// webrtc
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/log_sinks.h"
#include "rtc_base/string_utils.h"
#include "rtc_base/physical_socket_server.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/timestamp_aligner.h"
#include "rtc_base/logging.h"

//
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/task_queue/default_task_queue_factory.h"
#ifndef __OLD_WEBRTC_LIB__
  #include "api/video_track_source_proxy_factory.h"
#endif
#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"
#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#include "api/rtc_event_log/rtc_event_log_factory.h"
//
#include "media/engine/internal_decoder_factory.h"
#include "media/engine/internal_encoder_factory.h"
#include "media/engine/webrtc_media_engine.h"
#include "media/engine/webrtc_media_engine_defaults.h"
#include "media/base/adapted_video_track_source.h"
#include "media/base/video_adapter.h"
//
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/include/test_audio_device.h"
#include "modules/audio_device/include/audio_device_factory.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/audio_processing/include/audio_frame_proxies.h"
#include "modules/audio_mixer/audio_mixer_impl.h"
#include "modules/audio_device/include/audio_device_default.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"

#ifndef __OLD_WEBRTC_LIB__
#include <pc/video_track_source_proxy.h>
#else
#include <api/video_track_source_proxy.h>
#endif


//
#include "system_wrappers/include/sleep.h"

// json
#include <nlohmann/json.hpp>

// absl
#include <absl/memory/memory.h>
#include <absl/strings/match.h>

// yuv
#include <third_party/libyuv/include/libyuv.h>

// opengl
// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
// #define GLFW_INCLUDE_GLEXT
// #define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

// libevent2
#ifdef __OSX__
    #include <event2/event.h>
    #define __LIBEVENT_V2__
#else
    // use event included in libwebsocket.a
    #include <event.h>
#endif


#ifndef LWS_PROTOCOL_LIST_TERM
#define LWS_PROTOCOL_LIST_TERM { NULL, NULL, 0, 0, 0, NULL, 0 }
#endif
#define SIGNALING_SERVER            (getenv("SIGNALING_SERVER")==NULL?"192.168.10.10":getenv("SIGNALING_SERVER"))
#define WEBRTC_CHANNEL_PUBLISH      (getenv("WEBRTC_CHANNEL_PUBLISH")==NULL?"published":getenv("WEBRTC_CHANNEL_PUBLISH"))
#define WEBRTC_TOKEN                (getenv("WEBRTC_TOKEN")==NULL?"token-password":getenv("WEBRTC_TOKEN"))

#define CPUAFFINITY_MAIN            ((int)(getenv("CPUAFFINITY_MAIN")==NULL?1:atoi(getenv("CPUAFFINITY_MAIN"))))
#define CPUAFFINITY_TELNET          ((int)(getenv("CPUAFFINITY_TELNET")==NULL?2:atoi(getenv("CPUAFFINITY_TELNET"))))
#define CPUAFFINITY_BMDEV           ((int)(getenv("CPUAFFINITY_BMDEV")==NULL?3:atoi(getenv("CPUAFFINITY_BMDEV"))))

#define WND_WIDTH   (640)
#define WND_HEIGHT  (360)

#ifdef __BLACKMAGIC_OUTPUT__
#define WND_WIDTH_FHD   (WND_WIDTH*3)
#define WND_HEIGHT_FHD  (WND_HEIGHT*3)
#else
#define WND_WIDTH_FHD   (WND_WIDTH)
#define WND_HEIGHT_FHD  (WND_HEIGHT)
#endif

#ifndef MIN
#define MIN(a,b) (a>b?b:a)
#endif

#ifndef MAX
#define MAX(a,b) (a<b?b:a)
#endif

#ifndef htonll
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

#ifndef ETHER_MAX_LEN
#define ETHER_MAX_LEN   (1514)              /*!< ether パケット最大 */
#endif

#ifndef PANIC
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define PANIC(...)  PANIC_(__FILENAME__, __LINE__, __VA_ARGS__, "dummy")
#define PANIC_(func, line, format, ...) PANIC___(func, line, format "%.0s", __VA_ARGS__)
inline static void PANIC___(const char *funcname , const int line, const char *format, ...) {
    char msg_bf[512] = {0};
    int tmperr = errno;
    va_list ap;
    fprintf(stderr, "PANIC in %s [%d](%d:%s)\n", funcname, line, tmperr, strerror(tmperr));
    va_start(ap, format);
    vsnprintf(msg_bf, sizeof(msg_bf)-1,format, ap);
    vprintf(format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    syslog(LOG_ERR, ">> %20s<%s(%d:%s)", funcname, msg_bf, tmperr, strerror(tmperr));
    abort();
}
#endif

#define LOG(...)  LOG_(__FILENAME__, __LINE__, __VA_ARGS__, "dummy")
#define LOG_(func, line, format, ...) LOG___(func, line, format "%.0s", __VA_ARGS__)
inline static void LOG___(const char *funcname , const int line, const char *format, ...) {
    char msg_bf[512] = {0};
    int tmperr = errno;
    errno = 0;
    va_list ap;
    va_start(ap, format);
    vsnprintf(msg_bf, sizeof(msg_bf)-1,format, ap);
    va_end(ap);
    if (tmperr) {
        syslog(LOG_INFO, "%s[%4d]%p:%s(%d:%s)", funcname, line, (void*)pthread_self(), msg_bf, tmperr, strerror(tmperr));
    } else {
        syslog(LOG_INFO, "%s[%4d]%p:%s", funcname, line, (void*)pthread_self(), msg_bf);
    }
    errno = 0;
}

