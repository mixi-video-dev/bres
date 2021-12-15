#pragma once
#include "play_movie_ext.h"
//
namespace movie {
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
