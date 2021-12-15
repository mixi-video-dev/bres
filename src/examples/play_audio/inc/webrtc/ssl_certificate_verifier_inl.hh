#pragma once
#include "play_audio_ext.h"
//
namespace audio {
    //
    class SSLVerifier : public rtc::SSLCertificateVerifier {
    public:
        SSLVerifier(){}
        bool Verify(
            const rtc::SSLCertificate& certificate
        ) override
        {
            return true;
        }
    };
};
