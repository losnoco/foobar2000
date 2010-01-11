#include "../SDK/reader.h"

__int64 id3v2_calc_size(reader * r);//seeks to next byte after id3v2 tag

__int64 id3v2_remove(reader * r);//returns length of removed tag
