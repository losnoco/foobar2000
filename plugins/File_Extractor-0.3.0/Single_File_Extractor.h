
// Presents a single file as an "archive" of just that file

// File_Extractor 0.3.0

#ifndef SINGLE_FILE_EXTRACTOR_H
#define SINGLE_FILE_EXTRACTOR_H

#include "File_Extractor.h"

class Single_File_Extractor : public File_Extractor {
public:
	Single_File_Extractor();
	~Single_File_Extractor();
	
	void set_name( const char* );
	error_t extract( Data_Writer& out );
	error_t next();
	error_t rewind();
protected:
	error_t open_();
	void close_();
private:
	char name [256];
};

#endif

