#pragma once
#include "play_movie_ext.h"
//
namespace movie {
    enum OBSERVER_CLSID {
        OBSERVER_CLSID_MIN = 0,
        OBSERVER_CLSID_CreateSessionDescriptionObserver,
        OBSERVER_CLSID_SetSessionDescriptionObserver,
        OBSERVER_CLSID_SetSessionDescriptionObserverLocal,
        OBSERVER_CLSID_Instance,
        OBSERVER_CLSID_MAX
    };
    //
    class GetObserverInterface {
    public:
        virtual void* Get(
            OBSERVER_CLSID cls
        ) = 0;
    };
};
