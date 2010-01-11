#ifndef SONGINFO_H__
#define SONGINFO_H__

#include "nestypes.h"

#ifdef __cplusplus
extern "C" {
#endif

Uint SONGINFO_GetSongNo(void);
void SONGINFO_SetSongNo(Uint v);
Uint SONGINFO_GetStartSongNo(void);
void SONGINFO_SetStartSongNo(Uint v);
Uint SONGINFO_GetMaxSongNo(void);
void SONGINFO_SetMaxSongNo(Uint v);
Uint SONGINFO_GetExtendDevice(void);
void SONGINFO_SetExtendDevice(Uint v);
Uint SONGINFO_GetInitAddress(void);
void SONGINFO_SetInitAddress(Uint v);
Uint SONGINFO_GetPlayAddress(void);
void SONGINFO_SetPlayAddress(Uint v);
Uint SONGINFO_GetChannel(void);
void SONGINFO_SetChannel(Uint v);
Uint SONGINFO_GetInitializeLimiter(void);
void SONGINFO_SetInitializeLimiter(Uint v);

#ifdef __cplusplus
}
#endif

#endif /* SONGINFO_H__ */
