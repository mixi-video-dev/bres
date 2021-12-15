#include "play_movie_ext.h"

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
    #include <libswscale/swscale.h>
}

// global extern , instanciate.
movie::instance_t movie::instance_;
bool movie::quit_ = false;
std::vector<std::thread> movie::threads_;
std::mutex movie::mtx_;
std::queue<movie::sevent_t> movie::events_websockets_to_webrtc_;

namespace movie {
    typedef enum ITERATE_STATUS_ {
        ITERATE_STATUS_MIN = 0,
        READ_FRAME = ITERATE_STATUS_MIN,
        STREAM_IDX,
        SEND_PACKET,
        RECEIVE_FRAME,
        ITERATE_STATUS_MAX
    } ITERATE_STATUS;

    const AVCodec* codec = NULL;
    AVPacket* pkt = NULL;
    AVFrame* frame = NULL;
    AVFormatContext* fmtctx = NULL;
    AVCodecParserContext* parser = NULL;
    AVCodecContext* cdcctx = NULL;
    AVStream* stream = NULL;
    unsigned status = ITERATE_STATUS_MIN;
    //
    static bool iterate(
        std::function<bool(AVFrame*)> onFrame
    );
};
// Entry Point
int main(
    int argc,
    char *argv[]
)
{
    struct timeval tm;
    unsigned long prev_usec = 0;

    openlog(argv[0], LOG_PERROR|LOG_PID,LOG_LOCAL2);
    srand((unsigned) time(NULL) * getpid());

    // load movie
    if (avformat_open_input(&movie::fmtctx, MOVIE_FILE, NULL, NULL) != 0) {
        PANIC("avformat_open_input(%s)", MOVIE_FILE);
    }
    if (avformat_find_stream_info(movie::fmtctx, NULL) < 0) { PANIC("avformat_find_stream_info"); }
    for (auto i = 0; i < movie::fmtctx->nb_streams; ++i) {
        if (movie::fmtctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            movie::stream = movie::fmtctx->streams[i];
            break;
        }
    }
    if (movie::stream == NULL) { PANIC("No video stream ..."); }
    if ((movie::codec = avcodec_find_decoder(movie::stream->codecpar->codec_id)) == NULL){ PANIC("No supported decoder"); }
    if ((movie::cdcctx = avcodec_alloc_context3(movie::codec)) == NULL){ PANIC("avcodec_alloc_context3"); }
    if (avcodec_parameters_to_context(movie::cdcctx, movie::stream->codecpar) < 0) { PANIC("avcodec_parameters_to_context"); }
    if (avcodec_open2(movie::cdcctx, movie::codec, NULL) != 0) { PANIC("avcodec_open2\n"); }
    if ((movie::pkt = av_packet_alloc()) == NULL){ PANIC("av_packet_alloc"); }
    if ((movie::frame = av_frame_alloc()) == NULL){ PANIC("av_frame_alloc"); }
    //
    movie::instance_.videoRender = new movie::VideoRender();
    movie::instance_.audioRender = new movie::AudioRender();
    //
    movie::webrtcStart(&movie::instance_);
    movie::webSocketsStart(&movie::instance_);
    // websockets loop
    movie::threads_.push_back(std::thread(
        [&]{
            while(!movie::quit_) {
                lws_service(movie::instance_.context, 250);
            }
        }));
    //
    while (!movie::quit_){
        gettimeofday(&tm, NULL);
        auto now = tm.tv_sec*(uint64_t)1000000+tm.tv_usec;
        if (((now - prev_usec) >= 10000.0 /*  1ms */)) {
            movie::webrtcEvent(&movie::instance_);
            // movie play
            movie::iterate([](AVFrame* fr){
                auto h = fr->height;
                auto w = fr->width;
                auto r = fr->linesize[0];
                //
                std::vector<char> frameBuffer(h*w*4);

                unsigned rowbytes = (w * sizeof(unsigned));
                for (auto row = 0; row < h; ++ row) {
                    auto p0 = ((char*)frameBuffer.data()) + (row * rowbytes);
                    auto p1 = ((char*)fr->data[0] + (row * fr->linesize[0]));
                    memcpy(p0, p1, rowbytes);
                }
                //
                movie::instance_.videoRender->Render(
                    frameBuffer.data(),
                    frameBuffer.size(),
                    w, h
                );
                return(true);
            });
            prev_usec = now;
        }
    }
    movie::quit_ = true;
    for(auto &th : movie::threads_) {
        th.join();
    }
    movie::webrtcStop(&movie::instance_);
    movie::webSocketsStop(&movie::instance_);

    if (movie::fmtctx) { avformat_close_input(&movie::fmtctx); }
    if (movie::pkt) { av_packet_free(&movie::pkt); }
    if (movie::parser) { av_parser_close(movie::parser); }
    if (movie::cdcctx) { avcodec_free_context(&movie::cdcctx); }
    if (movie::frame) { av_frame_free(&movie::frame); }

    //
    closelog();
    return(0);
}
//
bool movie::iterate(
    std::function<bool(AVFrame*)> onFrame
)
{
    auto is_done = false;
    while(!is_done){
        if (movie::status == movie::READ_FRAME){
            if (av_read_frame(movie::fmtctx, movie::pkt) == 0) {
                movie::status = movie::STREAM_IDX;
            } else {
                is_done = true;
            }
        } else if (movie::status == movie::STREAM_IDX){
            if (movie::pkt->stream_index != movie::stream->index) {
                movie::status = READ_FRAME;
            } else {
                movie::status = SEND_PACKET;
            }
        } else if (movie::status == SEND_PACKET){
            if (avcodec_send_packet(movie::cdcctx, movie::pkt) != 0) {
                is_done = true;
            } else {
                movie::status = RECEIVE_FRAME;
            }
        } else if (movie::status == RECEIVE_FRAME){
            if (avcodec_receive_frame(movie::cdcctx, movie::frame) != 0){
                av_packet_unref(movie::pkt);
                movie::status = READ_FRAME;
            } else {
                if (!onFrame(movie::frame)){
                    PANIC("failed. onRead\n");
                }
                return(true);
            }
        }
    }
    return(!is_done);
}
