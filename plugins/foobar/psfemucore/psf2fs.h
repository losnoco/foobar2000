/////////////////////////////////////////////////////////////////////////////
//
// psf2 filesystem handling
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __PSF_PSF2FS_H__
#define __PSF_PSF2FS_H__

//
// Create or delete a psf2fs object
//
void *psf2fs_create(void);
void psf2fs_delete(void *psf2fs);

//
// Add an archive (.psf2) file to the virtual filesystem
// This will recursively handle _lib* tags as well.
//
// The "version" byte on all files must match the given compare_version.
//
// Returns >=0 on success, <0 on error
// 
//
int psf2fs_addarchive(void *psf2fs, const char *path, unsigned char compare_version);

//
// Read a part of a virtual file from the archives
//
// path   = a virtual pathname
// offset = byte offset within the virtual file
// buffer = output buffer
// length = requested number of bytes
//
// If length==0, this will do nothing and simply return the file length.
//
// On success, it returns the number of actually read bytes.
//
// If the file doesn't exist, or some other error occurs, it will return -1 and
// you can get the error message from psf2fs_getlasterror.
//
int psf2fs_virtual_readfile(void *psf2fs, const char *path, int offset, char *buffer, int length);

//
// Returns a pointer to a string describing the last error
//
const char *psf2fs_getlasterror(void *psf2fs);

void psf2fs_setabortcallback(void *psf2fs, abort_callback & p_abort);

#endif
