#pragma once
#include "play_movie_ext.h"
//
namespace movie {
    // classes
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
        virtual int32_t StartPlayout() override { return(0); }
        virtual int32_t StopPlayout() override { return(0); }
        virtual int32_t StartRecording() override { return(0); }
        virtual int32_t StopRecording() override { return(0); }
        virtual bool Playing() const override { return(false); }
        virtual bool Recording() const override { return(false); }
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
    };
};
