# bres example - simple/audio play

**bres-audio** , sample Implementation of Play Audio(only) to WebRTC Interface.

|key|desc|value|
|----|---|----|
|audio|wav|pcm_s16le ([1][0][0][0] / 0x0001), 48000 Hz, 2 channels|
|sfu|sora|https://sora.shiguredo.jp/|
|env|SIGNALING_SERVER|default:192.168.10.10|
|env|WEBRTC_CHANNEL_PUBLISH|default:audio-only|
|env|WEBRTC_TOKEN|default:token-password|
|env|AUDIO_PCM|default:test.wav|

## run

```
export SIGNALING_SERVER=xxxx
export WEBRTC_CHANNEL_PUBLISH=xxxx
export WEBRTC_METADATA=xxxx
export AUDIO_PCM=xxxxx
```

## compile/link

### osx

```
mkdir -p ./_build
cd ./_build
cmake ..
make -j
make
```

### ubuntu

TODO


