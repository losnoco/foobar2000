// RAR archive extractor

// File_Extractor 0.4.3
#ifndef RAR_EXTRACTOR_H
#define RAR_EXTRACTOR_H

#include "File_Extractor.h"

class Rar_Extractor : public File_Extractor {
public:
	Rar_Extractor();
	~Rar_Extractor();
	blargg_err_t extract( Data_Writer& );
protected:
	blargg_err_t open_();
	blargg_err_t next_();
	blargg_err_t rewind_();
	void close_();
private:
	class CmdExtract* impl;
	int index;
	bool can_extract;
	bool extracted;
	blargg_err_t next_item();
	blargg_err_t skip_item();
};

#endif
