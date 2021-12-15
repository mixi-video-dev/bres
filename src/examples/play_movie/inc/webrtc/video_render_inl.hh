#pragma once
#include "play_movie_ext.h"
//
namespace movie {
    // classes
    class VideoRender:
        public rtc::AdaptedVideoTrackSource
    {
    public:
        VideoRender(): AdaptedVideoTrackSource(4) {}
        virtual ~VideoRender(){}
        // rtc::AdaptedVideoTrackSource interface
        virtual bool is_screencast() const override { return(false); }
        virtual absl::optional<bool> needs_denoising() const override { return(false); }
        virtual webrtc::MediaSourceInterface::SourceState state() const override { return(SourceState::kLive); }
        virtual bool remote() const override { return(false); }
        // ATL-COM
        virtual void AddRef() const override {}
        virtual rtc::RefCountReleaseStatus Release() const override {
            return(rtc::RefCountReleaseStatus::kOtherRefsRemained);
        }
    public:
        void Render(char* img, unsigned length, unsigned width, unsigned height) {
            int64_t timems = rtc::TimeMillis();
            rtc::scoped_refptr<webrtc::I420Buffer> buffer = webrtc::I420Buffer::Create(width, height);
            rtc::scoped_refptr<webrtc::I420Buffer> scaled = webrtc::I420Buffer::Create(640, 360);
            buffer->InitializeData();
            //
            libyuv::ConvertToI420(
                (uint8_t*)img,
                length,
                buffer->MutableDataY(),
                buffer->StrideY(),
                buffer->MutableDataU(),
                buffer->StrideU(),
                buffer->MutableDataV(),
                buffer->StrideV(),
                0, 0,
                width, height,
                buffer->width(),
                buffer->height(),
                libyuv::kRotate0,
                libyuv::FOURCC_ARGB);
            //
            scaled->ScaleFrom(*buffer);
            // webrtc::I420Buffer::SetBlack(buffer.get());
            webrtc::VideoFrame frame = webrtc::VideoFrame::Builder()
                                        .set_video_frame_buffer(scaled)
                                        .set_timestamp_rtp(0)
                                        .set_timestamp_ms(rtc::TimeMillis())
                                        .set_timestamp_us(rtc::TimeMicros())
                                        .set_rotation(webrtc::kVideoRotation_0)
                                        .build();
            OnFrame(frame);
        }
    };
};
