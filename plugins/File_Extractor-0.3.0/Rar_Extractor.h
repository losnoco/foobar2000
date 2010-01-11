
// RAR archive extractor

// Rar_Extractor 0.3.0

#ifndef RAR_EXTRACTOR_H
#define RAR_EXTRACTOR_H

#include "File_Extractor.h"

class Rar_Extractor_Impl;

class Rar_Extractor : public File_Extractor {
public:
	Rar_Extractor();
	~Rar_Extractor();
	
	// documented in File_Extractor.h
	error_t next();
	File_Extractor::extract;
	error_t extract( Data_Writer& );
	error_t rewind();
	
protected:
	error_t open_();
	void close_();
private:
	Rar_Extractor_Impl* impl;
	bool extracted;
	error_t next_item();
};

#endif

