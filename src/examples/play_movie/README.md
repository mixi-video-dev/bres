# bres example - simple/movie play

**bres-movie** , sample Implementation of Play Movie(only) to WebRTC Interface.

|key|desc|value|
|----|---|----|
|movie|mov 4:3|rawvideo (raw  / 0x20776172), argb(top coded first (swapped))|
|sfu|sora|https://sora.shiguredo.jp/|
|env|SIGNALING_SERVER|default:192.168.10.10|
|env|WEBRTC_CHANNEL_PUBLISH|default:audio-only|
|env|WEBRTC_TOKEN|default:token-password|
|env|WEBRTC_METADATA|"metadata":{"access_token":"xxxxxxxx","stats":false}|
|env|MOVIE_FILE|default:test.mp4|

## dependencies

+ libwebsockets
+ libwebrtc
+ ffmpeg-dev
  + libavutil
  + libavcodec
  + libavformat
  + libswscale
  + libswresample



## run

```
export SIGNALING_SERVER=xxxx
export WEBRTC_CHANNEL_PUBLISH=xxxx
export WEBRTC_TOKEN=xxxx
export MOVIE_FILE=xxxxx
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


## note

### image access

![memory-copy](./res/memory-cache.png)
