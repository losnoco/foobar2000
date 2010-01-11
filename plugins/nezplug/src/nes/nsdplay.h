#ifndef NSDPLAY_H__
#define NSDPLAY_H__

#include "nestypes.h"

#ifdef __cplusplus
extern "C" {
#endif

Uint NSDPlayerInstall(Uint8 *pData, Uint uSize);
Uint32 NSDPlayerGetCycles(void);

#ifdef __cplusplus
}
#endif

#endif /* NSDPLAY_H__ */
