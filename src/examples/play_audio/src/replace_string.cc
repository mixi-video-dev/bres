#include "play_audio_ext.h"
//
std::string audio::replaceString(
      std::string src,
      std::string tgt, 
      std::string dst
)
{
    std::string::size_type  pos(src.find(tgt) );
    while(pos != std::string::npos) {
        src.replace(pos, tgt.length(), dst);
        pos = src.find(tgt, pos + dst.length() );
    }
    return(src);
}
