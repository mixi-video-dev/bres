#pragma once
#include "play_movie_ext.h"
//
namespace movie {
    //
    class VideoEncoderFactoryImpl : public webrtc::VideoEncoderFactory {
    public:
        VideoEncoderFactoryImpl() {}
        virtual ~VideoEncoderFactoryImpl() {}
    public:
        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override
        {
            std::vector<webrtc::SdpVideoFormat> supported_codecs;
            supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
            for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs()) {
                supported_codecs.push_back(format);
            }
            //
            return(supported_codecs);
        }
        //
        std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(
            const webrtc::SdpVideoFormat& format
        ) override
        {
            if (std::string(format.name).compare(cricket::kVp8CodecName) == 0) {
                return(webrtc::VP8Encoder::Create());
            }
            if (std::string(format.name).compare(cricket::kVp8CodecName) == 0) {
                return(webrtc::VP9Encoder::Create());
            }
            return(NULL);
        }
    };
    //
    class VideoDecoderFactoryImpl : public webrtc::VideoDecoderFactory {
    public:
        VideoDecoderFactoryImpl() {}
        virtual ~VideoDecoderFactoryImpl() {}
    public:
        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override
        {
            std::vector<webrtc::SdpVideoFormat> supported_codecs;
            supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
            for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs()) {
                supported_codecs.push_back(format);
            }
            return(supported_codecs);
        }
        //
        std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(
            const webrtc::SdpVideoFormat& format
        ) override
        {
            return webrtc::VP8Decoder::Create();
        }
    };
};