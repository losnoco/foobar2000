
/* Common interface to file archives */

/* File_Extractor 0.3.0 */

#ifndef FILE_EXTRACTOR_H
#define FILE_EXTRACTOR_H

#ifndef _MSC_VER
#include <stdint.h>
#else
typedef unsigned __int64 uint64_t;
#endif

/* C interface */

/* Only supports file to memory extraction. Also usable from C++. Functions documented
in C++ interface below. */
#ifdef __cplusplus
	extern "C" {
	class File_Extractor;
#else
	typedef struct File_Extractor File_Extractor;
#endif

/* 1. Open archive. Error is optionally returned in supplied pointer. */
File_Extractor* extractor_open_zip ( const char* path, const char** error ); /* .zip */
File_Extractor* extractor_open_7zip( const char* path, const char** error ); /* .7z */
File_Extractor* extractor_open_rar ( const char* path, const char** error ); /* .rar */
extern const char not_archive_err []; /* error when file is not archive of specified type */
/* Makes a normal file look like an archive containing just that file */
File_Extractor* extractor_open_file( const char* path, const char** error, const char* name );

/* 2. Operate on files in archive */
int         extractor_done     ( File_Extractor const* );
const char* extractor_name     ( File_Extractor const* );
long        extractor_size     ( File_Extractor const* );
uint64_t    extractor_timestamp( File_Extractor const* );
const char* extractor_extract  ( File_Extractor*, void* out, long count );
const char* extractor_next     ( File_Extractor* );
const char* extractor_rewind   ( File_Extractor* );
void        extractor_scan_only( File_Extractor* );

/* 3. Close archive and free memory used by extractor */
void extractor_close( File_Extractor* );

#ifdef __cplusplus
	}
#endif


/* C++ interface */
#ifdef __cplusplus

// All I/O goes through File_Reader and Data_Writer interfaces
#include "abstract_file.h"

class File_Extractor {
public:
	// Error string, NULL if no error
	typedef const char* error_t;
	
	// Open archive. Keeps pointer to input until close() but does not
	// delete reader at close() unless you call own_file() after open().
	error_t open( File_Reader* input );
	
	// True if end of archive has been reached
	int done() const            { return done_; }
	
	// Path of current file in archive, using '/' as directory separator
	const char* name() const    { return name_; }
	
	// Size of current file, in bytes
	long size() const           { return size_; }

	// Timestamp of current file
	uint64_t timestamp() const  { return timestamp_; }
	
	// Extract current file. Only valid operations after extraction
	// are next() and rewind(); you can't extract more than once.
	virtual error_t extract( void*, long n ); // extract first n bytes
	virtual error_t extract( Data_Writer& out ); // extract all data
	
	// Go to next file (skips directories)
	virtual error_t next() = 0;
	
// Additional features

	// Go back to first file in archive
	virtual error_t rewind() = 0;
	
	// Hint that no extraction will occur, which greatly speeds scanning
	// of solid RAR archives. Calling rewind() resets this.
	void scan_only() { scan_only_ = 1; }
	
	// Transfer ownership of current input file so that it will be deleted on close()
	void own_file() { own_file_ = 1; }
	
	// Close archive and free memory
	void close();
	
	File_Extractor() { init_state(); }
	virtual ~File_Extractor();
	
protected:
	// managed by derived classes
	const char* name_;
	long size_;
	uint64_t timestamp_;
	int done_;
	int scan_only_;
	File_Reader* reader;
	
	virtual error_t open_() = 0;
	virtual void close_() = 0;
private:
	int own_file_;
	
	void init_state();
};

File_Extractor* extractor_open_( File_Extractor*, const char* path, const char** error );

#endif

#endif

