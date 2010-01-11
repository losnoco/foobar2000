#ifndef __LOCAL_AVCODEC_H__
#define __LOCAL_AVCODEC_H__

#define HAVE_AV_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../wma2wav-0.1b2/ffmpeg-strip-wma/avcodec.h"
#include "../../../wma2wav-0.1b2/ffmpeg-strip-wma/avformat.h"

#ifdef __cplusplus
}
#endif

#ifdef malloc
#undef malloc
#undef realloc
#undef free
#endif

#endif
