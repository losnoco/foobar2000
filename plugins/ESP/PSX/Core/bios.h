/////////////////////////////////////////////////////////////////////////////
//
// bios - Holds BIOS image and can retrieve environment data
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __PSX_BIOS_H__
#define __PSX_BIOS_H__

#include "../../emuconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8* EMU_CALL bios_get_image_native(void);
uint32 EMU_CALL bios_get_imagesize(void);

sint32 EMU_CALL bios_init(void);

/*
** Find environment variables
** Returns nonzero on error
*/
sint32 EMU_CALL bios_getenv(
  const char *name,
  char *dest,
  sint32 dest_l
);

#ifdef __cplusplus
}
#endif

#endif
