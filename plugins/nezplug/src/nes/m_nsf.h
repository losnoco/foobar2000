#ifndef M_NSF_H__
#define M_NSF_H__

#include "nestypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* NSF player */
Uint NSFLoad(Uint8 *pData, Uint uSize);
Uint8 *NSFGetHeader(void);

#ifdef __cplusplus
}
#endif

#endif /* M_NSF_H__ */
