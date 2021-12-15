#include "bres+client_ext.h"
extern "C" {
  #include <SDL/SDL.h>
}
#ifdef __BLACKMAGIC_OUTPUT__
#include "bres+bm.hh"
#endif

namespace bres {
    static SDL_AudioSpec s_audioSpec;
    static std::vector<int16_t> buffer10ms;
    static std::vector<bres::instance_t>* pooled_inst = NULL;
    static void audioCallbackSDI(unsigned, unsigned&, unsigned char*, int);
};


void
bres::audioInit(
    void* arg
)
{
#ifndef __BLACKMAGIC_OUTPUT__
    SDL_AudioSpec wanted;
    wanted.freq = 48000;
    wanted.format = AUDIO_S16LSB;
    wanted.channels = 1;
    wanted.samples = 480;
    wanted.callback = audioCallback;
    wanted.userdata = arg;
    SDL_OpenAudio(&wanted, &s_audioSpec);
    SDL_PauseAudio(0);
#else
    std::vector<void*>& vv = *(std::vector<void*>*)(arg);
    if (vv.size() != 2) {
        PANIC("invalid args.");
    }
    bres::pooled_inst = (std::vector<bres::instance_t>*)(vv[0]);
    auto pdev = (bres::Device*)(vv[1]);
    if (pdev->_outputs.empty()) {
        PANIC("failed output is empty, or not enough");
    }
    for(auto& o : pdev->_outputs) {
        o->setAudioCallback(audioCallbackSDI);
    }
#endif
}
//
void bres::audioCallback(
    void* arg,
    unsigned char* data,
    int len
)
{
    bzero(data, len);
    std::vector<void*>& vv = *(std::vector<void*>*)(arg);
    if (vv.size() != 2) {
        PANIC("invalid args.");
    }
    std::vector<bres::instance_t>& pooled_instances = *(std::vector<bres::instance_t>*)(vv[0]);
    bres::instance_t& publish_instance = *(bres::instance_t*)(vv[1]);
    std::vector<int16_t> audioBuffer(480<<1 /* 2ch */);
    unsigned chann = 0;

    //
    for(auto& inst : pooled_instances) {
        if (inst.instance_enum != bres::INSTANCE_MIN) {
            bres::EVENT_TYPE type;
            std::string pcm;
            //
            if (bres::pop_event(
                &inst,
                bres::EVENT_ORDER_AUDIO,
                type,
                pcm))
            {
                if (inst.isDraw && pcm.length() == (480<<1)) {
                    for(auto n = 0; n < pcm.length(); n+=2) {
                        auto p = *(short*)((char*)pcm.data() + n);
                        auto chk = int(*((short*)&data[n]));
                        if ((chk + int(p)) > 32767){
                            *((short*)&data[n]) = (short)32767;
                        } else if ((chk + int(p)) < -32767){
                            *((short*)&data[n]) = (short)-32767;
                        } else {
                            *((short*)&data[n]) += (short)p;
                        }
                        audioBuffer[n] = *((short*)&data[n]);
                    }
                    chann++;
                }
            }
        }
    }
    if (chann && publish_instance.audioRender_) {
        buffer10ms.insert(buffer10ms.end(), audioBuffer.begin(), audioBuffer.end());
        if (buffer10ms.size() >= 960) {
            // transfer - audio
            publish_instance.audioRender_->Render(buffer10ms);
            buffer10ms.clear();
        }
    }
}
//
void bres::audioCallbackSDI(
    unsigned channel,
    unsigned& targetCount,
    unsigned char* data,
    int len
){
    bzero(data, len);
    targetCount = (len / AUDIO_OUT_CHANNELBITSZ);
    std::vector<bres::instance_t>& pooled_instances = *(bres::pooled_inst);
    //
    for(auto& inst : pooled_instances) {
        if (inst.instance_enum != bres::INSTANCE_MIN) {
            bres::EVENT_TYPE type;
            std::string pcm;
            //
            if (bres::pop_event(
                &inst,
                bres::EVENT_ORDER_AUDIO,
                type,
                pcm))
            {
                if (inst.isDraw && pcm.length() == (480<<1)) {
                    for(auto n = 0; n < pcm.length(); n+=2) {
                        auto p = *(short*)((char*)pcm.data() + n);
                        auto chk = int(*((short*)&data[n]));
                        if ((chk + int(p)) > 32767){
                            *((short*)&data[n]) = (short)32767;
                        } else if ((chk + int(p)) < -32767){
                            *((short*)&data[n]) = (short)-32767;
                        } else {
                            *((short*)&data[n]) += (short)p;
                        }
                    }
                }
            }
        }
    }
}
