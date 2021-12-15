#pragma once
#include "bres+client_def.h"

namespace bres {
    //
    class Capture:
        public rtc::AdaptedVideoTrackSource
    {
    public:
        Capture(): AdaptedVideoTrackSource(4) {}
        virtual ~Capture(){}
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
                libyuv::FOURCC_ABGR);
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
    //
    class AudioRender:
        public webrtc::webrtc_impl::AudioDeviceModuleDefault<webrtc::AudioDeviceModule>
    {
    public:
        void AddRef() const override {}
        rtc::RefCountReleaseStatus Release() const override {
            return(rtc::RefCountReleaseStatus::kDroppedLastRef);
        }
        AudioRender() { }
    public:
        virtual int32_t RegisterAudioCallback(webrtc::AudioTransport* callback) override {
            webrtc::MutexLock lock(&lock_);
            RTC_DCHECK(callback || audio_callback_);
            audio_callback_ = callback;
            return(0);
        }
        virtual int32_t StartPlayout() override {
            webrtc::MutexLock lock(&lock_);
            rendering_ = true;
            return(0);
        }
        virtual int32_t StopPlayout() override {
            webrtc::MutexLock lock(&lock_);
            rendering_ = false;
            return(0);
        }
        virtual int32_t StartRecording() override {
            webrtc::MutexLock lock(&lock_);
            capturing_ = true;
            return(0);
        }
        virtual int32_t StopRecording() override {
            webrtc::MutexLock lock(&lock_);
            capturing_ = false;
            return(0);
        }
        virtual bool Playing() const override {
            webrtc::MutexLock lock(&lock_);
            return(rendering_);
        }
        virtual bool Recording() const override {
            webrtc::MutexLock lock(&lock_);
            return(capturing_);
        }
    public:
        void Render(std::vector<int16_t>& data)
        {
            uint32_t newMicLevel = 0;
            if (audio_callback_) {
                audio_callback_->RecordedDataIsAvailable(
                        data.data(),
                        data.size()/2,
                        2 * 2, 2,
                        48000, 0, 0, 0, false, newMicLevel);
            }
        }
    private:
        mutable webrtc::Mutex lock_;
        webrtc::AudioTransport* audio_callback_;
        bool rendering_;
        bool capturing_;
    };
}; /* namespace scl */