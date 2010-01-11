
// ZIP archive extractor

// File_Extractor 0.3.0

#ifndef ZIP_EXTRACTOR_H
#define ZIP_EXTRACTOR_H

#include "File_Extractor.h"

class Zip_Extractor : public File_Extractor {
public:
	Zip_Extractor();
	~Zip_Extractor();
	
	// documented in File_Extractor.h
	error_t next();
	error_t extract( void*, long );
	error_t rewind();
	
protected:
	error_t open_();
	void close_();
private:
	char* catalog;
	long offset;
	char name_copy [256];
	
	void update_info();
	void next_item();
};

#endif

