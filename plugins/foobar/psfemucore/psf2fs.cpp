/////////////////////////////////////////////////////////////////////////////
//
// psf2 filesystem handling
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include <foobar2000.h>

#include "psf2fs.h"

#include "tag.h"

/////////////////////////////////////////////////////////////////////////////

#define MYMAXPATH (1024)

struct SOURCE_FILE {
  char *name;
  int reserved_size;
  struct SOURCE_FILE *next;
};

struct DIR_ENTRY {
  char name[37];
  struct DIR_ENTRY *subdir;
  int length;
  int block_size;
  struct SOURCE_FILE *source;
  int *offset_table;
  struct DIR_ENTRY *next;
};

struct CACHEBLOCK {
  struct SOURCE_FILE *from_source;
  int   from_offset;
  char *uncompressed_data;
  int   uncompressed_size;
};

#define ERRORSTRINGMAX (256)

struct PSF2FS {
  struct SOURCE_FILE *sources;
  struct DIR_ENTRY *dir;
  struct CACHEBLOCK cacheblock;

  // ahboy!
  abort_callback * p_abort;

  char errorstring[ERRORSTRINGMAX];
  int adderror;
};

/////////////////////////////////////////////////////////////////////////////

static void source_cleanup_free(struct SOURCE_FILE *source) {
  while(source) {
    struct SOURCE_FILE *next = source->next;
    if(source->name) delete [] source->name;
    delete [] source;
    source = next;
  }
}

static void dir_cleanup_free(struct DIR_ENTRY *dir) {
  while(dir) {
    struct DIR_ENTRY *next = dir->next;
    if(dir->subdir) dir_cleanup_free(dir->subdir);
    delete [] dir;
    dir = next;
  }
}

static void cache_cleanup(struct CACHEBLOCK *cacheblock) {
  if(!cacheblock) return;
  if(cacheblock->uncompressed_data) delete [] cacheblock->uncompressed_data;
}

/////////////////////////////////////////////////////////////////////////////

void *psf2fs_create(void) {
  struct PSF2FS *fs;
  fs = new PSF2FS;
  if(!fs) return NULL;
  memset(fs, 0, sizeof(struct PSF2FS));
  return fs;
}

/////////////////////////////////////////////////////////////////////////////

void psf2fs_delete(void *psf2fs) {
  struct PSF2FS *fs = (struct PSF2FS*)psf2fs;
  if(fs->sources) source_cleanup_free(fs->sources);
  if(fs->dir) dir_cleanup_free(fs->dir);
  cache_cleanup(&(fs->cacheblock));
}

/////////////////////////////////////////////////////////////////////////////

static void errormessageadd(struct PSF2FS *fs, const char *message) {
  int k = strlen(fs->errorstring);
  int l = ERRORSTRINGMAX - k;
  char *e = fs->errorstring + k;
  strncpy(e, message, l);
  e[l-1] = 0;
}

static void errormessagethrown(struct PSF2FS *fs, const char *fn, const char *message) {
  errormessageadd(fs, "exception thrown in");
  errormessageadd(fs, fn);
  errormessageadd(fs, " - ");
  errormessageadd(fs, message );
}

static void errormessage(struct PSF2FS *fs, const char *message) {
  fs->errorstring[0] = 0;
  errormessageadd(fs, message);
}

/////////////////////////////////////////////////////////////////////////////

static int isdirsep(char c) { return (c == '/' || c == '\\' || c == '|' || c == ':'); }

static void makelibpath(const char *path, const char *libpath, char *finalpath, int finalpath_length) {
  int l;
  int p_l = 0;
  for(l = 0; path[l]; l++) { if(isdirsep(path[l])) { p_l = l + 1; } }
  while(isdirsep(*libpath)) libpath++;
  if(!finalpath_length) return;
  *finalpath = 0;
  if(p_l > (finalpath_length - 1)) p_l = (finalpath_length - 1);
  if(p_l) {
    memcpy(finalpath, path, p_l);
    finalpath[p_l] = 0;
    finalpath += p_l;
    finalpath_length -= p_l;
  }
  if(!finalpath_length) return;
  strncpy(finalpath, libpath, finalpath_length);
  finalpath[finalpath_length - 1] = 0;
}

/////////////////////////////////////////////////////////////////////////////

static unsigned read32lsb(const service_ptr_t<file> & f, abort_callback & p_abort) {
  unsigned char foo[4];
  f->read_object(foo, 4, p_abort);
  return (
    ((foo[0] & 0xFF) <<  0) |
    ((foo[1] & 0xFF) <<  8) |
    ((foo[2] & 0xFF) << 16) |
    ((foo[3] & 0xFF) << 24)
  );
}

/////////////////////////////////////////////////////////////////////////////

static struct DIR_ENTRY *finddirentry(
  struct DIR_ENTRY *dir,
  const char *name,
  int name_l
) {
  if(name_l > 36) return NULL;
  while(dir) {
    if(!memicmp(dir->name, name, name_l) && dir->name[name_l] == 0) return dir;
    dir = dir->next;
  }
  return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// Make a DIR_ENTRY list for a given file and Reserved offset.
// Recurses subdirectories also.
// All entries are set to point to the given SOURCE_FILE.
//
static struct DIR_ENTRY *makearchivedir(
  struct PSF2FS *fs,
  const service_ptr_t<file> & f,
  int offset,
  struct SOURCE_FILE *source
) {
  struct DIR_ENTRY *dir = NULL;
  int n, num;
  if(offset < 0) goto corrupt;
  if(offset >= source->reserved_size) { errormessageadd(fs, ""); goto corrupt; }
  if((offset + 4) > source->reserved_size) { errormessageadd(fs, ""); goto corrupt; }
  try {
  f->seek(16 + offset, *fs->p_abort);
  num = read32lsb(f, *fs->p_abort);
  offset += 4;
  if(num < 0) goto corrupt;
  for(n = 0; n < num; n++) {
    int o, u, b;
    if((offset + 48) > source->reserved_size) { errormessageadd(fs, ""); goto corrupt; }
    { struct DIR_ENTRY *entry = new DIR_ENTRY;
      if(!entry) goto outofmemory;
      memset(entry, 0, sizeof(struct DIR_ENTRY));
      entry->next = dir;
      dir = entry;
    }
	f->seek(16 + offset, *fs->p_abort);
	f->read_object(dir->name, 36, *fs->p_abort);
    o = read32lsb(f, *fs->p_abort);
    u = read32lsb(f, *fs->p_abort);
    b = read32lsb(f, *fs->p_abort);
    offset += 48;
    if(o < 0) goto corrupt;
    if(u < 0) goto corrupt;
    if(b < 0) goto corrupt;
    if(o && o < offset) { 
//      char s[100];
//      sprintf(s,"q[o=%08X offset=%08X]",o,offset);
//      errormessageadd(fs, s);
      goto corrupt;
    }
    // if this new entry describes a subdirectory:
    if(u == 0 && b == 0 && o != 0) {
      dir->subdir = makearchivedir(fs, f, o, source);
      if(fs->adderror) goto error;
//      if(!dir->subdir) goto error;
    // if this new entry describes a zero-length file:
    } else if(u == 0 || b == 0 || o == 0) {
      // fields were zero anyway
    // if this new entry describes a real source file:
    } else {
      int i;
      int blocks = (u + (b-1)) / b;
      int dataofs = o + 4 * blocks;
      if(dataofs >= source->reserved_size) { errormessageadd(fs, ""); goto corrupt; }
      // record the info
      dir->length = u;
      dir->block_size = b;
      dir->source = source;
      dir->offset_table = new int[blocks + 1];
      if(!dir->offset_table) goto outofmemory;
      for(i = 0; i < blocks; i++) {
        int cbs;
        if((o + 4) > source->reserved_size) { errormessageadd(fs, ""); goto corrupt; }
		f->seek(16 + o, *fs->p_abort);
        cbs = read32lsb(f, *fs->p_abort);
        o += 4;
        dir->offset_table[i] = dataofs;
        dataofs += cbs;
      }
      dir->offset_table[i] = dataofs;
    }
  }
  }
  catch(std::exception const & e) {
    errormessagethrown(fs, "makearchivedir", e.what());
    goto error;
  }
  catch(...) {
    errormessagethrown(fs, "makearchivedir", "unknown");
    goto error;
  }
success:
  return dir;

corrupt:
  errormessageadd(fs, "Virtual directory is corrupt");
  goto error;
outofmemory:
  errormessageadd(fs, "Out of memory");
  goto error;
error:
  dir_cleanup_free(dir);
  fs->adderror = 1;
  return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// Merge two SOURCE_FILE lists.
// Guaranteed to succeed and not to free anything.
//
static struct SOURCE_FILE *mergesource(
  struct SOURCE_FILE *to,
  struct SOURCE_FILE *from
) {
  struct SOURCE_FILE *to_tail;
  if(!to && !from) return NULL;
  if(!to) {
    struct SOURCE_FILE *t;
    t = to; to = from; from = t;
  }
  to_tail = to;
  while(to_tail->next) { to_tail = to_tail->next; }
  to_tail->next = from;
  return to;
}

/////////////////////////////////////////////////////////////////////////////
//
// Merge two DIR_ENTRY lists.
// Guaranteed to succeed. May free some structures.
//
static struct DIR_ENTRY *mergedir(
  struct DIR_ENTRY *to,
  struct DIR_ENTRY *from
) {
  // will traverse "from", and add to "to".
  while(from) {
    struct DIR_ENTRY *entry_to;
    struct DIR_ENTRY *entry_from;
    entry_from = from;
    from = from->next;
    // delink entry_from
    entry_from->next = NULL;
    // look for a duplicate entry in "to"
    entry_to = finddirentry(to, entry_from->name, strlen(entry_from->name));
    // if there is one, do something fancy and then free entry_from.
    if(entry_to) {
      // if both are subdirs, merge the subdirs
      if((entry_to->subdir) && (entry_from->subdir)) {
        entry_to->subdir = mergedir(entry_to->subdir, entry_from->subdir);
        entry_from->subdir = NULL;
      // if both are files, copy over the info
      } else if((!(entry_to->subdir)) && (!(entry_from->subdir))) {
        entry_to->length = entry_from->length;
        entry_to->block_size = entry_from->block_size;
        entry_to->source = entry_from->source;
        if(entry_to->offset_table) delete [] entry_to->offset_table;
        entry_to->offset_table = entry_from->offset_table;
        entry_from->offset_table = NULL;
      // if one's a subdir but the other's not, we lose "from". this is fine.
      }
      dir_cleanup_free(entry_from);
      entry_from = NULL;
    // otherwise, just relink to the top of "to"
    } else {
      entry_from->next = to;
      to = entry_from;
    }
  }
  return to;
}

/////////////////////////////////////////////////////////////////////////////
//
// only modifies *psource and *pdir on success
//
static int addarchive(
  struct PSF2FS *fs,
  const char *path,
  unsigned char compare_version,
  int level,
  struct SOURCE_FILE **psource,
  struct DIR_ENTRY **pdir
) {
  char *tag = NULL;
  service_ptr_t<file> f;
  int fl, tag_ofs, tag_bytes;
  t_filesize fl64;
  unsigned char header[16];
  unsigned char tagmarker[5];
  int reserved_size;
  int program_size;
  int skip_tag = 0;
  int libnum = 1;
  // these will accumulate info from the various _libs
  struct SOURCE_FILE *source = NULL;
  struct DIR_ENTRY *dir = NULL;
  // these relate to the current file
  struct SOURCE_FILE *this_source = NULL;
  struct DIR_ENTRY *this_dir = NULL;

  // default to no error
  fs->adderror = 0;

  // Error message prefix
  errormessage(fs, level ? "While loading a _lib file: " : "");

  if(level >= 10) {
    errormessageadd(fs, "Recursion limit reached");
    goto error;
  }

dofile:
  try { filesystem::g_open(f, path, filesystem::open_mode_read, *fs->p_abort); }
  catch ( std::exception const & e ) {
    errormessageadd(fs, "Unable to open ");
	errormessageadd(fs, path);
	errormessageadd(fs, " - ");
	errormessageadd(fs, e.what() );
    goto error;
  }
  catch ( ... ) {
    errormessageadd(fs, "Unable to open ");
	errormessageadd(fs, path);
	errormessageadd(fs, " - ");
	errormessageadd(fs, "unknown" );
    goto error;
  }
  try { fl64 = f->get_size(*fs->p_abort); }
  catch ( std::exception const & e ) {
    errormessageadd(fs, "Unable to query size for ");
	errormessageadd(fs, path);
	errormessageadd(fs, " - ");
	errormessageadd(fs, e.what() );
    goto error;
  }
  catch ( ... ) {
	errormessageadd(fs, "Unable to query size for ");
	errormessageadd(fs, path);
	errormessageadd(fs, " - ");
	errormessageadd(fs, "unknown" );
    goto error;
  }
  if (fl64 > 1<<30)
  {
	  errormessageadd(fs, path);
	  errormessageadd(fs, " is too large.");
	  goto error;
  }
  fl = (int)fl64;
  if(fl < 16) goto invalidformat;
  try { f->read_object(header, 16, *fs->p_abort); }
  catch ( std::exception const & e ) {
    errormessageadd(fs, "Unable to read ");
	errormessageadd(fs, path);
	errormessageadd(fs, " - ");
	errormessageadd(fs, e.what() );
    goto error;
  }
  catch ( ... ) {
    errormessageadd(fs, "Unable to read ");
	errormessageadd(fs, path);
	errormessageadd(fs, " - ");
	errormessageadd(fs, "unknown" );
    goto error;
  }
  if(
    (memcmp(header, "PSF", 3)) ||
    (header[3] != compare_version)
  ) {
    goto invalidformat;
  }

  reserved_size =
    ((((int)(header[ 4])) & 0xFF) <<  0) |
    ((((int)(header[ 5])) & 0xFF) <<  8) |
    ((((int)(header[ 6])) & 0xFF) << 16) |
    ((((int)(header[ 7])) & 0xFF) << 24);
  program_size =
    ((((int)(header[ 8])) & 0xFF) <<  0) |
    ((((int)(header[ 9])) & 0xFF) <<  8) |
    ((((int)(header[10])) & 0xFF) << 16) |
    ((((int)(header[11])) & 0xFF) << 24);

  if(reserved_size < 0) goto invalidformat;
  if(program_size < 0) goto invalidformat;
  if(reserved_size > fl) goto invalidformat;
  if(program_size > fl) goto invalidformat;

  tag_ofs = 16 + reserved_size + program_size;
  if(tag_ofs < 16) goto invalidformat;
  if(tag_ofs > fl) goto invalidformat;
  tag_bytes = fl - (tag_ofs + 5);
  if(tag_bytes > 50000) tag_bytes = 50000;
  if(tag_bytes <= 0) goto notag;

  if(skip_tag) goto notag;

  try {
	f->seek(tag_ofs, *fs->p_abort);
	f->read_object(tagmarker, 5, *fs->p_abort);
  } catch (...) { goto notag; }
  if(memcmp(tagmarker, "[TAG]", 5)) goto notag;
  tag = new char[tag_bytes + 1];
  if(!tag) goto outofmemory;
  tag[tag_bytes] = 0;
  try { f->read_object(tag, tag_bytes, *fs->p_abort); }
  catch ( std::exception const & e ) {
    errormessageadd(fs, "Error reading tag");
	errormessageadd(fs, " - ");
	errormessageadd(fs, e.what() );
    goto error;
  }
  catch ( ... ) {
    errormessageadd(fs, "Error reading tag");
	errormessageadd(fs, " - ");
	errormessageadd(fs, "unknown" );
    goto error;
  }
  f.release();
  for(libnum = 1;; libnum++) {
    char libstr[64];
    char value[MYMAXPATH];
    char finalpath[MYMAXPATH];
    strcpy(libstr, "_lib");
    if(libnum > 1) sprintf(libstr + strlen(libstr), "%d", libnum);
    if(tag_getvar(tag, libstr, value, sizeof(value)) < 0) break;
    if(!value[0]) break;
    makelibpath(path, value, finalpath, sizeof(finalpath));
    if(addarchive(fs, finalpath, compare_version, level + 1, &source, &dir) < 0) goto error;
  }
  skip_tag = 1;
  goto dofile;

notag:
  // create a source entry for this psf2
  this_source = new SOURCE_FILE;
  if(!this_source) goto outofmemory;
  memset(this_source, 0, sizeof(struct SOURCE_FILE));
  this_source->name = new char[strlen(path) + 1];
  if(!this_source->name) goto outofmemory;
  strcpy(this_source->name, path);
  this_source->reserved_size = reserved_size;
  this_dir = makearchivedir(fs, f, 0, this_source);
  if(fs->adderror) goto error;
//  if(!this_dir) goto error;
  f.release();

  // success
  // now merge everything
  *psource = mergesource(source, this_source);
  *pdir = mergedir(dir, this_dir);
success:
  return 0;

invalidformat:
  errormessageadd(fs, "File format is invalid");
  goto error;
outofmemory:
  errormessageadd(fs, "Out of memory");
  goto error;
error:
  if(dir) dir_cleanup_free(dir);
  if(source) source_cleanup_free(source);
  if(this_dir) dir_cleanup_free(this_dir);
  if(this_source) source_cleanup_free(this_source);
  if(tag) delete [] tag;
  return -1;
}

/////////////////////////////////////////////////////////////////////////////
//
//
//
int psf2fs_addarchive(void *psf2fs, const char *path, unsigned char compare_version) {
  struct PSF2FS *fs = (struct PSF2FS*)psf2fs;
  return addarchive(fs, path, compare_version, 0, &(fs->sources), &(fs->dir));
}

/////////////////////////////////////////////////////////////////////////////
//
//
//
static int virtual_read(struct PSF2FS *fs, struct DIR_ENTRY *entry, int offset, char *buffer, int length) {
  service_ptr_t<file> f;
  char *zdata = NULL;
  int length_read = 0;
  int r;
  int destlen;
  if(offset >= entry->length) return 0;
  if((offset + length) > entry->length) length = entry->length - offset;
  while(length_read < length) {
    // get info on the current block
    int blocknum = offset / entry->block_size;
    int ofs_within_block = offset % entry->block_size;
    int canread;
    int block_zofs  = entry->offset_table[blocknum];
    int block_zsize = entry->offset_table[blocknum+1] - block_zofs;
    int block_usize;
    if(block_zofs <= 0 || block_zofs >= entry->source->reserved_size) goto bounds;
    if((block_zofs+block_zsize) > entry->source->reserved_size) goto bounds;

    // get the actual uncompressed size of this block
    block_usize = entry->length - (blocknum * entry->block_size);
    if(block_usize > entry->block_size) block_usize = entry->block_size;

    // if it's not already in the cache block, read it
    if(
      (fs->cacheblock.from_offset != block_zofs) ||
      (fs->cacheblock.from_source != entry->source)
    ) {
      zdata = new char[block_zsize];
      if(!zdata) goto outofmemory;
	  try { filesystem::g_open(f, entry->source->name, filesystem::open_mode_read, *fs->p_abort); }
  catch ( std::exception const & e ) {
    errormessage(fs, "Unable to open ");
	errormessageadd(fs, entry->source->name);
	errormessageadd(fs, " - ");
	errormessageadd(fs, e.what() );
    goto error;
  }
  catch ( ... ) {
    errormessage(fs, "Unable to open ");
	errormessageadd(fs, entry->source->name);
	errormessageadd(fs, " - ");
	errormessageadd(fs, "unknown" );
    goto error;
  }
  try { f->seek(16 + block_zofs, *fs->p_abort); }
  catch ( std::exception const & e ) {
	    errormessage(fs, "Error seeking");
		errormessageadd(fs, " - ");
	errormessageadd(fs, " - ");
	errormessageadd(fs, e.what() );
    goto error;
  }
  catch ( ... ) {
	    errormessage(fs, "Error seeking");
		errormessageadd(fs, " - ");
	errormessageadd(fs, " - ");
	errormessageadd(fs, "unknown" );
    goto error;
  }
  try { f->read_object(zdata, block_zsize, *fs->p_abort); }
  catch ( std::exception const & e ) {
        errormessage(fs, "Error reading virtual block");
		errormessageadd(fs, " - ");
	errormessageadd(fs, " - ");
	errormessageadd(fs, e.what() );
    goto error;
  }
  catch ( ... ) {
        errormessage(fs, "Error reading virtual block");
		errormessageadd(fs, " - ");
	errormessageadd(fs, " - ");
	errormessageadd(fs, "unknown" );
    goto error;
  }
	  f.release();

      // invalidate cache without freeing buffer
      fs->cacheblock.from_source = NULL;

      // make sure there's a buffer allocated
      // but only reallocate if the size is different
      if(fs->cacheblock.uncompressed_size != block_usize) {
        fs->cacheblock.uncompressed_size = 0;
        if(fs->cacheblock.uncompressed_data) {
          delete [] fs->cacheblock.uncompressed_data;
          fs->cacheblock.uncompressed_data = NULL;
        }
        fs->cacheblock.uncompressed_data = new char[block_usize];
        if(!fs->cacheblock.uncompressed_data) goto outofmemory;
        fs->cacheblock.uncompressed_size = block_usize;
      }
      destlen = block_usize;
      // attempt decompress
      r = uncompress((unsigned char *) fs->cacheblock.uncompressed_data, (unsigned long *) &destlen, (const unsigned char *) zdata, block_zsize);
      delete [] zdata;
      if(r != Z_OK || destlen != block_usize) {
//        char s[999];
//        sprintf(s,"zdata=%02X %02X %02X blockz=%d blocku=%d destlenout=%d", zdata[0], zdata[1], zdata[2], block_zsize, block_usize, destlen);
        errormessage(fs, "Error decompressing virtual block");
//        errormessageadd(fs, s);
        goto error;
      }
    }

    // at this point, we can read whatever we want out of the cacheblock
    canread = fs->cacheblock.uncompressed_size - ofs_within_block;
    if(canread > (length - length_read)) canread = length - length_read;

    // copy
    memcpy(buffer, fs->cacheblock.uncompressed_data + ofs_within_block, canread);

    // advance pointers/counters
    offset += canread;
    length_read += canread;
    buffer += canread;
  }

success:
  return length_read;

bounds:
  errormessage(fs, "Virtual file block out of bounds");
  goto error;
outofmemory:
  errormessage(fs, "Out of memory");
  goto error;
error:
  // if cacheblock was invalidated, we can free it
  if(!fs->cacheblock.from_source) {
    fs->cacheblock.uncompressed_size = 0;
    if(fs->cacheblock.uncompressed_data) {
      delete [] fs->cacheblock.uncompressed_data;
      fs->cacheblock.uncompressed_data = NULL;
    }
  }
  if(zdata) delete [] zdata;
  return -1;
}

/////////////////////////////////////////////////////////////////////////////
//
//
//
int psf2fs_virtual_readfile(void *psf2fs, const char *path, int offset, char *buffer, int length) {
  struct PSF2FS *fs = (struct PSF2FS*)psf2fs;
  struct DIR_ENTRY *entry = fs->dir;


  if(!path) goto invalidarg;
  if(offset < 0) goto invalidarg;
  if(!buffer) goto invalidarg;
  if(length < 0) goto invalidarg;

  for(;;) {
    int l;
    int need_dir;
    if(!entry) goto pathnotfound;
    while(isdirsep(*path)) path++;
    for(l = 0;; l++) {
      if(!path[l]) { need_dir = 0; break; }
      if(isdirsep(path[l])) { need_dir = 1; break; }
    }
    entry = finddirentry(entry, path, l);
    if(!entry) goto pathnotfound;
    if(!need_dir) break;
    entry = entry->subdir;
    path += l;
  }

  // if we "found" a file but it's a directory, then we didn't find it
  if(entry->subdir) goto pathnotfound;

  // special case: if requested length is 0, return the total file length
  if(!length) return entry->length;

  // otherwise, read from source
  return virtual_read(fs, entry, offset, buffer, length);

pathnotfound:
  errormessage(fs, "Path not found");
  goto error;
invalidarg:
  errormessage(fs, "Invalid argument");
  goto error;
error:
  return -1;
}

/////////////////////////////////////////////////////////////////////////////

const char *psf2fs_getlasterror(void *psf2fs) {
  struct PSF2FS *fs = (struct PSF2FS*)psf2fs;
  return (const char*)(fs->errorstring);
}

/////////////////////////////////////////////////////////////////////////////

void psf2fs_setabortcallback(void *psf2fs, abort_callback & p_abort) {
  struct PSF2FS *fs = (struct PSF2FS*)psf2fs;
  fs->p_abort = &p_abort;
}

/////////////////////////////////////////////////////////////////////////////
