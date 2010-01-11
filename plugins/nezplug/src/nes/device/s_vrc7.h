#ifndef S_VRC7_H__
#define S_VRC7_H__

#include "../nestypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USE_FMOPL 0

void VRC7SoundInstall(void);
void VRC7SetTone(void *p, Uint type);

#ifdef __cplusplus
}
#endif

#endif /* S_VRC7_H__ */
