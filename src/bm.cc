#include "bres+bm.hh"
#include "bres+client_ext.h"

#ifdef __BLACKMAGIC_OUTPUT__

namespace bres {
    static std::deque<char*> _pipelineIngress;
    static std::deque<char*> _pipelineEgress;
    static std::mutex _mtx;
};



// =============================
// Device Implemented
// =============================
bres::Device::Device(void* parent):_parent(parent),_cnt(0)
    ,_deckLinkDiscovery(CreateDeckLinkDiscoveryInstance())
{
    LOG("DeviceDiscovery::DeviceDiscovery(interface : %p/%p/%p)", _deckLinkDiscovery, this, parent);
    if (_deckLinkDiscovery){
        _deckLinkDiscovery->InstallDeviceNotifications(this);
    }
}
bres::Device::~Device(){
    if (_deckLinkDiscovery){
        _deckLinkDiscovery->UninstallDeviceNotifications();
        _deckLinkDiscovery->Release();
    }
}

HRESULT bres::Device::DeckLinkDeviceArrived(IDeckLink* link){
    const char *txt = NULL;
    std::string disp_s, model_s;
    HRESULT ret;
    //
    if ((ret = link->GetDisplayName(&txt)) != S_OK){ return(ret); }
    disp_s = txt;
    free((void*)txt);
    if ((ret = link->GetModelName(&txt)) != S_OK){ return(ret); }
    model_s = txt;
    free((void*)txt);
    //
    LOG("DeckLinkDeviceArrived : %s : %s(%p/%p)", disp_s.c_str(), model_s.c_str(), link, this);
    {
        LOCK lock(_mtx);
        if (_cnt == 0) {
            LOG(">>output : %s : %s(%p/%p/%u)",
                disp_s.c_str(),
                model_s.c_str(),
                link,
                this,
                _cnt
            );
            _outputs.push_back(new Output(this, link, _cnt));
        }
        _cnt++;
    }
    return(S_OK);
}
HRESULT bres::Device::DeckLinkDeviceRemoved(IDeckLink* link){
    const char *txt = NULL;
    std::string disp_s, model_s;
    HRESULT ret;
    if ((ret = link->GetDisplayName(&txt)) != S_OK){ return(ret); }
    disp_s = txt;
    free((void*)txt);
    if ((ret = link->GetModelName(&txt)) != S_OK){ return(ret); }
    model_s = txt;
    free((void*)txt);

    LOG("DeckLinkDeviceRemoved : %s : %s/%p",
        disp_s.c_str(),
        model_s.c_str(),
        link
    );
    {
        LOCK lock(_mtx);
        for(auto it = _outputs.begin();it != _outputs.end();){
            if ((*it)->_link == link){
                it = _outputs.erase(it);
            }else{
                it ++;
            }
        }
    }
    return(S_OK);
}
//
void bres::Device::setFrame(
    unsigned id,
    char* frame,
    unsigned framelen,
    unsigned width,
    unsigned height,
    unsigned pixel
)
{
    LOCK lock(_mtx);
    if (id >= _outputs.size()){
        PANIC("invalid vector size");
    }
    _outputs.at(id)->setFrame(frame, framelen, width ,height, pixel);
}

// =============================
// Output Implemented
// =============================
bres::Output::Output(
    Device* device,
    IDeckLink* link,
    unsigned cnt
):  _link(link),
    _parent(device),
    _lastFrame(NULL),
    _cnt(cnt),
    _bufferedSamples(0),
    _linkOutput(NULL),
    _dispMode(0)
{
    IDeckLinkDisplayMode* displayMode = NULL;
    IDeckLinkMutableVideoFrame* outputFrame = NULL;
    void* pframe;
    struct timespec ts,start_ts;

    if(clock_gettime(
        CLOCK_MONOTONIC,
        &start_ts) != 0)
    {
        PANIC("clock_gettime");
    }
    //
	if (link->QueryInterface(
        IID_IDeckLinkOutput,
        (void**)&_linkOutput) != S_OK)
    {
        PANIC("QueryInterface : IID_IDeckLinkOutput");
    }
    _linkOutput->SetScheduledFrameCompletionCallback(this);
    _linkOutput->SetAudioCallback(this);
    if (_linkOutput->GetDisplayMode(
        bmdModeHD1080i5994,
        &displayMode) != S_OK)
    {
        PANIC("GetDisplayModeIterator");
    }
    _width = (unsigned)displayMode->GetWidth();
    _height = (unsigned)displayMode->GetHeight();
    _dispMode = displayMode->GetDisplayMode();
    //
    displayMode->GetFrameRate(
        &_duration,
        &_timescale
    );
    _perSecond = (_timescale + (_duration-1))  /  _duration;
    if (_duration == 1001 && _timescale % 30000 == 0){
        _dropFrames = 2 * (_timescale / 30000);
    }else{
        _dropFrames = 0;
    }
    if(clock_gettime(
        CLOCK_MONOTONIC,
        &ts) != 0)
    {
        PANIC("clock_gettime");
    }
    LOG("display : 0x%08x / %u / %u / %u / %u / %u\nnsec : %u/ sec : %u", 
        (_dispMode&bmdSupportedVideoModeKeying),
        _width,
        _height,
        _perSecond,
        (unsigned)_duration,
        (unsigned)_timescale,
        (unsigned)ts.tv_nsec,
        (unsigned)ts.tv_sec);
    //
    if (_linkOutput->EnableVideoOutput(
                        displayMode->GetDisplayMode(),
                        bmdVideoOutputVITC) != S_OK)
    {
        PANIC("EnableVideoOutput");
    }
    if (_linkOutput->EnableAudioOutput(
                        bmdAudioSampleRate48kHz,
                        bmdAudioSampleType16bitInteger,
                        2,  // stereo
                        bmdAudioOutputStreamContinuous) != S_OK)
    {
        PANIC("EnableAudioOutput");
    }
    _totalFramesScheduled = 0;
    for (unsigned int i = 0; i < 10; i++){
        if (_linkOutput->CreateVideoFrame(
                            _width,
                            _height,
                            _width * 4,
                            bmdFormat8BitBGRA,
                            bmdFrameFlagDefault,
                            &outputFrame) != S_OK)
        {
            PANIC("CreateVideoFrame");
        }
        // initialize black
        outputFrame->GetBytes((void**)&pframe);
        memset(
            pframe,
            0xFF,
            outputFrame->GetRowBytes() * displayMode->GetHeight()
        );
        if (i < 5 /* half */) {
            if (_linkOutput->ScheduleVideoFrame(
                                outputFrame,
                                (_totalFramesScheduled * _duration),
                                _duration,
                                _timescale) != S_OK)
            {
                PANIC("_linkOutput->ScheduleVideoFrame");
            }
        } else {
            _completedFrames.push_back(outputFrame);
        }
        _totalFramesScheduled++;
    }
	displayMode->Release();
    _lastFrame = (char*)malloc(_width * 4 * _height);
    memset(_lastFrame, 0xFF, (_width * 4 * _height));
    _linkOutput->StartScheduledPlayback(0, _timescale, 1.0);
    _linkOutput->BeginAudioPreroll();
}
//
bres::Output::~Output(){
    if (_linkOutput){
        _linkOutput->StopScheduledPlayback(0, NULL, 0);
        _linkOutput->SetScreenPreviewCallback(nullptr);
        _linkOutput->DisableAudioOutput();
        _linkOutput->DisableVideoOutput();
        _linkOutput->SetAudioCallback(NULL);
        _linkOutput->SetScheduledFrameCompletionCallback(NULL);
        _linkOutput->Release();
    }
    if (_lastFrame){
        free(_lastFrame);
    }
    for(auto& frame : _outputFrames){
        frame->Release();
    }
    for(auto& frame : _completedFrames){
        frame->Release();
    }
}
//
int bres::Output::setFrame(
    char* frame,
    unsigned framelen,
    unsigned width,
    unsigned height,
    unsigned pixel
)
{
    LOCK lock(_mtx);
    void* pframe;
    if (_completedFrames.empty()){
        return(E_OUTOFMEMORY);
    }
    IDeckLinkMutableVideoFrame* outframe = _completedFrames.front();
    _completedFrames.pop_front();
    _outputFrames.push_back(outframe);
    //
    outframe->GetBytes((void**)&pframe);
    memcpy(pframe, frame, framelen);

    _width = width;
    _height = height;
    //
    return(S_OK);
}
//
HRESULT bres::Output::ScheduledFrameCompleted(
    IDeckLinkVideoFrame* device,
    BMDOutputFrameCompletionResult completionResult
)
{
    LOCK lock(_mtx);
    IDeckLinkMutableVideoFrame* outframe;
    void* pframe;
    // 
    _completedFrames.push_back((IDeckLinkMutableVideoFrame*)(device));
    //
    if (_outputFrames.empty()){
        outframe = _completedFrames.front();
        _completedFrames.pop_front();
        outframe->GetBytes((void**)&pframe);
        memcpy(pframe, _lastFrame, _width*_height*4);
    }else{
        outframe = _outputFrames.front();
        _outputFrames.pop_front();
        outframe->GetBytes((void**)&pframe);
        memcpy(_lastFrame, pframe, _width*_height*4);
    }
    //
    if (completionResult == bmdOutputFrameDisplayedLate || 
        completionResult == bmdOutputFrameDropped){
        _totalFramesScheduled += 2;
    }
    if (_linkOutput->ScheduleVideoFrame(
                        outframe,
                        (_totalFramesScheduled * _duration),
                        _duration,
                        _timescale) != S_OK)
    {
        PANIC("_linkOutput->ScheduleVideoFrame");
    }
    _totalFramesScheduled++;
    //
    return(S_OK);
}
//
HRESULT bres::Output::ScheduledPlaybackHasStopped(){
    LOG("Output::ScheduledPlaybackHasStopped");
    return(S_OK);
}
//
HRESULT bres::Output::RenderAudioSamples(
    bool preroll
)
{
	unsigned int		bufferedSamples;
    unsigned int		samplesWritten;

    if ((_linkOutput->GetBufferedAudioSampleFrameCount(&bufferedSamples) == S_OK) && 
        (bufferedSamples < AUDIO_RATE48K /* 48k */)){
        LOCK lock(_mtx);
        unsigned trgtcnt = 0;
        auto remain = (AUDIO_RATE48K /* 48k */ - bufferedSamples);
        auto datalen = (remain * AUDIO_OUT_CHANNELBITSZ);
        auto data = (unsigned char*)malloc(datalen);
        bzero(data, datalen);
        // bufferdSamples
        _bufferedSamples = bufferedSamples;
        // 
        if (_audioCallback) {
            _audioCallback(_cnt, trgtcnt, data, datalen);
        }
        if (trgtcnt > 0){
            if (_linkOutput->ScheduleAudioSamples(data, trgtcnt, 0, 0, &samplesWritten) != S_OK){
                LOG("failed. ScheduleAudioSamples : (%u)", trgtcnt);
            }
            if (trgtcnt != samplesWritten){
                LOG("drop. ScheduleAudioSamples : (%u/%u)", trgtcnt, samplesWritten);
            }
        }
        free(data);
    }else{
        struct timespec ts;
        if(clock_gettime(CLOCK_MONOTONIC,&ts) != 0) {
            PANIC("clock_gettime");
        }
        LOG(
            "GetBufferedAudioSampleFrameCount : (%u) %8u.%09u",
            bufferedSamples,
            (unsigned)ts.tv_sec,
            (unsigned)ts.tv_nsec);
    }
    if (preroll){
        LOG("Output::RenderAudioSamples : preroll");
        _linkOutput->StartScheduledPlayback(0, _timescale, 1.0);
    }
    return(S_OK);
}
//
int bres::Output::setAudioCallback(
    std::function<void(unsigned,unsigned&,unsigned char*,int)> audioCallback
)
{
    _audioCallback = audioCallback;
    return(S_OK);
}
//
void
bres::bm_pipeline_service(
    void* arg
)
{
    auto pdev = (bres::Device*)arg;
    std::vector<std::vector<char> > pipelineFrames;

    // allocate pipeline frames
    for(auto n = 0;n < 32; n++){ 
        pipelineFrames.push_back(std::vector<char>(WND_WIDTH_FHD * WND_HEIGHT_FHD*4));
    }
    {
        LOCK lock(bres::_mtx);
        for(auto& f : pipelineFrames){
            bres::_pipelineIngress.push_back(f.data());
        }
        bres::_pipelineEgress.clear();
    }
    // blackmagic pipeline entory point
    while(!bres::quit) {
        char* pframe = NULL;
        {
            LOCK lock(bres::_mtx);
            if (!bres::_pipelineEgress.empty()) {
                pframe = bres::_pipelineEgress.front();
                bres::_pipelineEgress.pop_front();
            }
        }
        if (pframe) {
            pdev->setFrame(
                0,
                pframe,
                WND_WIDTH_FHD*WND_HEIGHT_FHD*4,
                WND_WIDTH_FHD,
                WND_HEIGHT_FHD,
                4
            );
            bres::_pipelineIngress.push_back(
                pframe
            );
        } else {
            usleep(100);;
        }
    }
}
//
void
bres::bm_add_pipeline_frame(
    const char* frame,
    unsigned framelen
)
{
    char* pframe = NULL;
    {
        LOCK lock(bres::_mtx);
        if (bres::_pipelineIngress.empty()) {
            // LOG("empty ingress buffer.");
            return;
        }
        pframe = bres::_pipelineIngress.front();
        bres::_pipelineIngress.pop_front();
    }
    if (!pframe) {
        PANIC("must be check compile/flag(rtti)");
    }
    if (framelen != (WND_WIDTH_FHD * WND_HEIGHT_FHD * 4)) {
        PANIC("invalid size(%u != %u)", framelen , (WND_WIDTH_FHD * WND_HEIGHT_FHD * 4));
    }
    memcpy(pframe, frame, MIN(WND_WIDTH_FHD * WND_HEIGHT_FHD * 4, framelen));
    {
        LOCK lock(bres::_mtx);
        bres::_pipelineEgress.push_back(pframe);
    }
}

#endif // __BLACKMAGIC_OUTPUT__
