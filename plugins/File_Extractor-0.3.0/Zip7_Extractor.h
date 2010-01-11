
// 7-zip archive extractor

// Zip7_Extractor 0.3.0

#ifndef ZIP7_EXTRACTOR_H
#define ZIP7_EXTRACTOR_H

#include "File_Extractor.h"

struct Zip7_Extractor_Impl;

class Zip7_Extractor : public File_Extractor {
public:
	Zip7_Extractor();
	~Zip7_Extractor();
	
	// documented in File_Extractor.h
	error_t next();
	error_t extract( Data_Writer& out );
	error_t rewind();
	
protected:
	error_t open_();
	void close_();
private:
	Zip7_Extractor_Impl* impl;
	int index;
};

#endif

