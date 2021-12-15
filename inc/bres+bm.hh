#pragma once
#include "bres+client_def.h"

#ifdef __BLACKMAGIC_OUTPUT__
#include "DeckLinkAPI.h"

#define NOTUSE_IUNKNON_FUNCTION   \
    virtual HRESULT QueryInterface(REFIID, LPVOID*){ return(S_OK); }\
    virtual ULONG AddRef() { return(0); }\
    virtual ULONG Release() { return(0); }

//
namespace bres{
    class Device;
    //
    class Output: public IDeckLinkVideoOutputCallback,
                    public IDeckLinkAudioOutputCallback
    {
    public:
        void* _parent;
        IDeckLinkOutput* _linkOutput;
        IDeckLink* _link;
        unsigned _width, _height, _dispMode, _cnt, _bufferedSamples;
        BMDTimeValue _duration;
        BMDTimeScale _timescale;
        uint32_t _perSecond;
        uint32_t _dropFrames;
        unsigned _totalFramesScheduled;
        unsigned _firstCompletaion;
        char* _lastFrame;
        std::deque<IDeckLinkMutableVideoFrame*> _outputFrames;
        std::deque<IDeckLinkMutableVideoFrame*> _completedFrames;
        std::mutex _mtx;
        std::function<void(unsigned,unsigned&,unsigned char*,int)> _audioCallback;
    public:
        Output(
            Device* device, 
            IDeckLink* link, 
            unsigned cnt
        );
        virtual ~Output();
    public:
        int setFrame(
            char* frame,
            unsigned framelen,
            unsigned width,
            unsigned height,
            unsigned pixel
        );
        int setAudioCallback(
            std::function<void(unsigned,unsigned&,unsigned char*,int)> audioCallback
        );
    public:
        // IDeckLinkVideoOutputCallback
        virtual HRESULT ScheduledFrameCompleted(
            IDeckLinkVideoFrame* device,
            BMDOutputFrameCompletionResult completionResult
        );
        virtual HRESULT ScheduledPlaybackHasStopped();
        // IDeckLinkAudioOutputCallback
        virtual HRESULT RenderAudioSamples(
            bool preroll
        );
        NOTUSE_IUNKNON_FUNCTION
    };
    //
    class Device : public IDeckLinkDeviceNotificationCallback{
    public:
        void* _parent;
        IDeckLinkDiscovery* _deckLinkDiscovery;
        std::vector<Output*> _outputs;
        std::mutex _mtx;
        unsigned _cnt;
    public:
        Device(void* parent);
        virtual ~Device();
    public:
        void setFrame(
            unsigned id,
            char* frame,
            unsigned framelen,
            unsigned width,
            unsigned height,
            unsigned pixel
        );
    public:
        // IDeckLinkDeviceArrivalNotificationCallback interface's
        virtual HRESULT DeckLinkDeviceArrived(
            IDeckLink* link
        );
        virtual HRESULT DeckLinkDeviceRemoved(
           IDeckLink* link
        );
        NOTUSE_IUNKNON_FUNCTION
    };
}; // namespace bres
#endif // __BLACKMAGIC_OUTPUT__
