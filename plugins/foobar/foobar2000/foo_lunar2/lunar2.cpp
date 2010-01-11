/*
	changelog

2005-10-08 21:10 UTC - kode54
- Updated to 0.9 beta 7 + input API

2003-09-12 09:07 UTC - kode54
- Simplified test_filename() mess, fixing archive support
- Version is now 0.5

2003-06-26 07:04 - kode54
- Updated to 0.7 API
- Version is now 0.4

2003-04-09 16:23 - kode54
- Added bitspersample info
- File name is no longer hard coded
- Version is now 0.3

*/

#include "../SDK/foobar2000.h"

static const unsigned char sums[][16] = {
	{14,3,227,103,215,65,171,182,101,136,123,106,59,249,70,128},
	{130,231,215,26,192,18,77,39,3,155,230,74,229,87,209,109},
	{84,94,56,155,40,148,185,197,237,129,211,21,247,179,50,148},
	{81,7,30,184,7,155,46,120,45,117,32,64,105,139,24,249},
	{70,51,76,230,255,64,0,233,59,155,162,218,168,118,46,42},
	{18,12,22,32,238,65,236,126,38,104,107,11,59,223,69,194},
	{69,174,231,172,28,193,234,83,50,136,233,195,98,121,28,187},
	{38,249,175,70,197,159,193,120,255,83,217,189,22,129,105,154},
	{172,19,46,40,30,179,70,98,214,179,206,148,64,172,158,153},
	{151,69,238,86,5,186,83,222,62,35,74,33,75,107,129,209},
	{193,48,37,185,139,188,137,189,110,152,94,166,90,25,123,93},
	{0,13,74,61,26,8,106,27,182,149,251,155,215,161,253,85},
	{59,55,46,71,193,157,215,162,28,163,18,217,66,122,195,97},
	{50,169,202,103,109,143,41,15,151,206,81,173,216,232,178,18},
	{127,141,35,104,203,182,99,182,195,217,194,72,203,243,139,41},
	{158,91,15,34,187,136,144,59,183,133,126,5,55,201,50,178},
	{221,30,126,102,98,75,157,232,50,103,16,20,185,231,204,77},
	{137,246,14,57,30,184,201,160,47,31,18,192,187,108,108,9},
	{140,116,79,79,99,175,150,92,25,17,178,82,196,50,66,143},
	{28,216,216,200,140,33,2,25,113,129,162,89,183,155,207,251},
	{32,173,8,225,197,190,117,97,245,54,145,128,93,57,65,177},
	{164,155,150,213,230,158,86,4,71,231,165,198,73,125,191,103},
	{95,249,16,111,209,245,199,144,111,218,234,79,166,169,197,198},
	{15,238,202,78,15,181,119,40,35,216,231,212,191,215,155,226},
	{234,207,212,176,216,89,105,184,89,105,101,214,219,185,28,233},
	{210,7,153,213,222,138,123,50,98,251,217,182,131,175,13,123},
	{13,84,151,172,48,122,6,250,57,80,229,204,249,151,79,11},
	{185,15,102,155,141,99,75,11,131,100,58,20,24,126,86,118},
	{147,126,220,106,8,217,220,177,21,227,161,155,188,236,206,233},
	{179,244,185,127,98,148,40,37,115,114,236,238,111,55,97,5},
	{166,15,34,103,248,24,186,49,174,224,173,208,119,207,194,44},
	{62,183,83,152,69,135,3,150,164,6,222,92,169,141,250,221},
	{47,64,108,118,250,4,146,174,11,108,147,31,206,132,109,192},
	{186,16,92,135,255,21,101,187,31,14,209,77,53,246,91,184},
	{179,131,134,86,38,174,2,181,225,213,105,64,234,133,121,106},
	{135,67,162,254,141,133,31,55,221,225,153,94,179,221,235,99},
	{223,171,153,40,183,191,121,113,188,2,0,125,209,50,153,13},
	{219,59,18,181,87,201,71,141,52,122,36,229,121,60,248,174},
	{43,225,234,165,100,255,166,41,85,163,172,202,149,145,62,194},
	{232,214,19,148,136,178,50,81,59,126,223,116,109,116,194,240},
	{17,116,70,176,178,69,128,153,133,24,198,137,22,60,240,194},
	{244,114,129,159,169,245,44,128,219,29,178,126,13,182,218,234},
	{185,1,201,57,197,192,85,84,54,25,113,71,190,115,151,131},
	{243,47,158,190,112,75,176,95,75,249,46,226,229,107,39,182},
	{170,94,254,11,225,251,215,75,200,252,223,88,236,114,220,212},
	{70,138,21,215,238,45,6,190,102,250,242,54,108,231,222,55},
	{210,19,4,41,248,155,185,81,89,115,87,144,171,216,42,30},
	{22,18,16,149,188,133,114,80,142,177,140,74,65,92,77,35},
	{180,188,56,163,115,99,144,174,129,110,153,171,153,73,127,66},
	{125,17,5,180,131,148,56,17,117,1,178,88,66,71,7,245},
	{62,163,53,14,143,87,100,40,159,113,87,84,36,134,203,158},
	{63,20,8,46,77,10,73,92,65,75,57,26,85,34,69,246},
	{233,124,126,2,251,41,134,118,146,97,102,169,196,79,88,135},
	{129,72,171,145,249,21,56,44,186,151,122,205,59,160,95,232},
	{58,14,116,251,11,154,203,207,211,97,196,226,9,69,105,241},
	{44,25,233,148,210,90,83,159,248,157,156,0,195,187,85,243},
	{28,26,55,205,6,52,97,230,144,8,226,241,223,116,189,204},
	{39,47,176,157,58,146,3,153,160,11,136,174,181,190,84,103},
	{237,137,214,179,159,164,171,76,152,3,61,95,205,58,76,214},
	{204,66,203,186,27,210,50,180,34,157,139,29,172,239,254,227},
	{10,66,6,156,102,6,225,33,49,174,126,225,168,200,139,171},
	{198,247,218,55,215,42,225,249,196,85,214,189,92,170,89,27},
	{161,192,96,94,70,244,45,122,114,25,189,38,17,101,228,13},
	{239,5,7,59,196,201,81,53,165,6,119,232,69,48,89,74},
	{200,189,204,146,48,100,202,121,213,211,27,98,139,216,47,192},
	{145,247,93,200,89,25,154,193,251,105,160,138,97,91,28,84},
	{39,235,84,33,69,84,108,10,199,154,177,242,235,60,125,150},
	{132,6,195,199,166,94,221,214,22,88,204,103,76,68,212,14},
	{154,190,233,85,29,171,108,115,167,212,70,133,184,45,222,167},
	{199,81,21,139,94,116,173,158,153,253,146,244,50,170,208,250},
//	{84,94,56,155,40,148,185,197,237,129,211,21,247,179,50,148},
	{230,129,155,207,38,29,91,89,69,223,216,218,30,17,125,35},
	{252,242,162,243,27,141,182,10,244,179,213,12,164,140,191,93},
	{67,121,26,162,174,196,141,202,75,75,230,214,57,141,212,164},
	{72,52,171,51,51,118,106,50,16,46,100,189,239,247,140,14},
	{2,62,44,54,243,56,49,224,75,40,44,232,74,224,38,226},
	{255,58,236,94,237,88,183,171,68,167,232,96,227,13,167,18},
	{88,223,32,207,126,97,78,89,178,20,70,59,98,46,57,52},
	{253,36,245,113,219,113,108,146,176,144,123,82,153,160,242,88},
	{227,58,240,111,57,145,23,125,98,153,137,74,58,67,27,22},
//	{84,94,56,155,40,148,185,197,237,129,211,21,247,179,50,148},
	{210,40,20,77,121,204,127,56,192,2,179,37,78,12,6,150},
	{253,66,20,161,45,54,171,237,10,56,111,31,36,177,82,153},
//	{84,94,56,155,40,148,185,197,237,129,211,21,247,179,50,148},
	{163,157,32,71,121,93,79,255,199,67,126,178,218,62,59,239},
	{222,141,32,152,209,31,222,238,11,18,245,231,249,101,158,108},
	{131,73,244,209,116,61,218,83,91,236,30,121,170,161,46,98},
	{65,145,78,217,81,78,30,190,125,140,89,231,6,232,26,128},
	{82,55,126,81,34,118,155,80,19,193,77,231,162,197,185,236},
	{157,158,138,15,63,39,244,142,106,40,34,170,5,7,126,195},
	{238,98,174,182,129,161,76,246,59,26,184,207,219,140,155,174},
	{186,202,254,74,125,151,23,210,222,53,188,253,181,217,177,119},
	{141,43,103,204,95,66,167,145,205,239,36,218,111,130,58,104},
	{127,254,143,0,207,109,249,108,80,6,15,105,63,199,114,231},
//	{84,94,56,155,40,148,185,197,237,129,211,21,247,179,50,148},
	{114,70,23,111,185,137,178,66,34,181,201,113,117,160,24,231},
	{235,139,49,91,192,75,182,156,223,70,193,41,75,148,132,98},
	{96,33,200,5,176,109,160,193,142,131,43,74,170,10,206,117},
	{10,2,160,32,218,22,220,135,3,42,111,63,192,50,68,215},
	{67,199,110,94,161,37,146,125,144,20,53,0,91,253,163,150},
	{163,91,191,101,55,152,202,108,5,82,143,243,158,108,175,91},
	{31,91,73,27,11,30,111,147,29,145,182,59,71,11,252,134},
//	{145,247,93,200,89,25,154,193,251,105,160,138,97,91,28,84},
	{118,41,56,241,48,208,219,44,8,249,65,129,162,178,154,44},
	{68,203,119,199,136,137,244,85,124,115,251,136,234,147,105,186},
	{20,129,175,241,93,96,75,205,101,117,53,129,95,34,238,78},
	{167,212,4,9,60,61,140,61,168,243,119,95,22,157,113,121},
	{235,199,14,249,11,91,23,104,23,75,138,114,7,66,163,196},
	{193,101,77,28,103,13,90,118,178,204,239,139,87,254,44,176},
	{214,201,77,140,250,142,102,246,158,110,130,24,208,245,34,68},
	{192,181,85,236,175,167,176,200,125,72,76,111,46,122,35,83},
	{28,234,178,10,210,159,40,78,174,110,117,17,238,217,129,152},
	{194,77,193,250,187,60,165,236,62,30,62,201,248,117,254,19},
	{90,80,229,158,189,173,16,113,140,80,68,56,31,125,202,228},
	{245,44,91,109,214,125,186,11,1,214,244,232,13,228,29,81},
//	{210,19,4,41,248,155,185,81,89,115,87,144,171,216,42,30},
	{234,219,5,127,28,237,139,26,119,89,152,166,186,162,164,107},
	{173,0,146,152,96,35,201,25,0,120,205,5,160,176,255,107},
	{45,10,223,212,109,133,65,127,129,0,233,78,173,129,99,4},
	{188,16,160,49,100,191,243,177,201,118,211,185,166,67,24,196},
	{29,148,108,57,64,108,65,193,138,180,165,4,166,234,94,154},
	{220,94,96,94,14,146,79,123,178,187,35,245,238,63,9,30},
	{231,56,236,18,192,53,80,120,164,49,148,13,36,79,81,114},
	{227,191,175,164,103,70,33,16,195,85,151,59,213,63,58,102},
//	{84,94,56,155,40,148,185,197,237,129,211,21,247,179,50,148},
	{109,140,79,116,245,234,201,187,73,236,178,243,143,41,92,95},
//	{84,94,56,155,40,148,185,197,237,129,211,21,247,179,50,148},
	{207,219,221,110,229,243,76,248,206,42,85,13,50,200,107,165},
//	{84,94,56,155,40,148,185,197,237,129,211,21,247,179,50,148},
	{98,113,106,111,161,246,94,232,173,50,8,134,78,219,31,244},
	{83,31,242,38,153,44,147,124,177,48,88,79,36,188,220,238},
	{169,220,81,24,117,37,138,238,32,59,218,199,193,92,73,226},
	{247,123,21,222,119,168,8,249,26,40,238,144,43,247,200,0},
	{19,196,235,41,202,175,207,185,49,79,124,135,78,84,144,238},
	{73,80,217,110,136,45,147,97,34,18,254,24,68,231,157,87},
	{229,98,22,1,232,209,69,64,128,76,161,30,170,233,88,170},
	{2,18,190,24,40,120,84,196,82,97,202,79,232,89,139,137},
	{72,24,91,215,252,56,230,95,143,99,229,148,233,21,214,225},
	{16,173,86,141,165,23,250,33,253,125,72,190,62,86,38,87},
	{28,189,68,222,183,37,174,219,44,77,222,3,17,75,39,67},
	{161,117,176,175,87,134,97,80,251,199,193,52,44,131,41,48},
//	{84,94,56,155,40,148,185,197,237,129,211,21,247,179,50,148},
	{215,0,15,248,69,42,207,198,45,164,222,206,231,10,128,51},
	{193,28,175,249,255,10,98,252,175,129,56,107,26,20,11,157},
	{228,51,153,147,244,65,21,223,131,69,69,184,88,22,91,101},
	{118,191,152,39,157,223,60,64,63,58,3,62,87,140,210,75},
	{38,238,108,189,181,75,47,28,160,99,31,115,232,182,173,137},
	{253,203,82,166,142,208,164,248,76,50,57,57,200,128,200,201},
	{225,97,97,183,174,158,254,39,199,86,240,94,63,15,174,47},
	{207,246,62,40,247,6,65,65,59,55,160,212,101,101,95,10},
	{83,232,67,174,64,201,216,22,241,243,133,169,43,67,60,28},
	{0,91,170,27,85,158,27,82,36,197,33,219,159,221,74,170},
//	{84,94,56,155,40,148,185,197,237,129,211,21,247,179,50,148},
	{224,0,8,221,157,196,190,68,38,168,155,53,248,78,251,162},
	{12,200,138,148,25,166,25,17,183,20,157,111,122,12,244,100},
	{213,149,175,233,77,1,36,195,199,93,68,134,209,153,162,132},
	{173,122,169,30,217,106,61,83,177,65,229,42,124,25,61,200},
	{46,49,172,253,94,72,250,44,208,129,148,15,102,206,176,12},
	{226,80,55,210,0,199,6,243,55,13,113,28,122,155,123,82},
	{95,207,208,236,143,16,96,117,23,183,56,119,59,32,16,208},
	{47,110,113,44,13,220,103,162,191,217,44,41,38,136,134,126},
	{247,22,201,75,146,88,242,168,135,72,2,16,187,236,3,209},
	{102,100,166,105,197,49,132,148,130,238,133,129,249,88,2,228},
	{124,104,179,240,45,35,174,116,253,1,116,84,141,22,16,157},
	{56,51,98,15,184,70,40,13,140,182,152,215,95,173,236,91},
	{119,75,169,115,113,91,255,182,235,100,65,164,149,43,110,190},
	{12,132,224,47,235,24,19,134,33,85,181,145,27,120,244,81},
	{31,108,218,54,49,2,171,168,106,220,250,219,200,141,202,117},
	{185,80,252,175,78,40,151,39,109,106,231,107,110,11,15,222},
	{6,214,156,236,231,69,88,220,6,188,195,172,63,35,203,76},
	{108,36,66,1,12,142,54,147,56,133,126,244,205,173,210,42},
	{217,215,184,146,65,49,101,72,31,118,78,236,201,73,204,178},
	{229,77,201,78,110,106,154,99,171,176,160,220,221,56,23,220},
	{167,152,29,249,168,197,75,111,113,253,38,41,70,204,162,121},
	{86,111,125,178,42,90,37,173,88,133,156,128,0,167,132,93},
	{100,156,63,44,40,238,206,38,200,14,49,120,113,232,100,233},
	{225,61,185,189,25,216,30,234,11,125,154,132,65,98,248,153},
	{171,190,140,104,59,81,18,23,99,210,62,63,251,82,179,53},
	{203,161,234,140,76,190,18,133,140,120,35,204,105,40,83,30},
	{178,135,1,196,104,38,46,29,152,112,3,203,103,24,174,142},
	{141,255,58,50,96,229,218,102,249,138,184,251,253,191,253,234},
	{205,132,37,182,199,46,201,12,116,131,209,154,168,212,212,112},
	{139,125,223,233,64,70,87,10,150,38,70,109,57,74,29,160},
	{75,181,141,234,53,209,103,190,10,192,95,152,105,142,227,170},
	{50,247,217,89,207,253,113,236,91,140,78,95,172,224,151,249},
	{93,188,114,156,165,162,190,35,116,225,51,217,206,71,105,19},
	{209,103,23,212,56,210,216,102,170,171,241,139,215,121,40,121},
	{149,36,244,49,20,156,67,237,138,248,78,185,204,129,80,128},
	{69,238,126,171,205,168,90,52,178,251,24,153,98,149,88,35},
	{224,227,71,136,38,24,198,221,251,185,107,30,58,145,255,99},
	{148,7,249,135,135,199,192,149,193,90,13,104,170,79,225,15},
	{53,254,161,112,243,70,152,168,219,124,184,43,176,202,230,180},
	{99,86,13,34,255,186,157,114,78,198,59,254,182,225,170,232},
	{72,114,105,196,184,138,22,30,102,65,150,213,38,223,95,96},
	{35,31,145,238,255,104,192,95,104,176,114,34,6,101,134,40},
	{94,126,214,173,1,102,65,81,191,253,102,8,24,135,223,175},
	{31,203,137,215,143,109,105,63,196,60,243,112,190,84,133,28},
	{58,189,58,246,19,228,124,187,208,18,3,21,43,10,253,158},
	{101,81,246,239,21,58,112,61,128,14,20,205,233,70,186,37},
	{49,67,93,170,14,214,208,147,218,178,161,24,130,254,7,28},
	{113,190,51,17,211,24,224,131,72,255,31,191,19,192,154,221},
	{200,230,65,87,36,16,107,107,94,186,77,209,96,108,229,53},
	{195,109,178,86,163,213,86,159,97,44,28,53,178,229,226,205},
	{95,6,210,242,212,69,76,223,3,173,230,13,73,99,223,73},
	{251,233,46,226,50,92,247,228,189,90,209,62,238,62,246,213},
	{105,99,250,99,209,102,184,168,137,202,10,222,28,178,42,181},
	{19,238,127,195,1,13,253,28,81,193,200,58,245,236,135,40},
	{248,43,201,144,222,82,185,125,211,142,19,142,90,106,58,141},
	{194,29,158,50,93,230,33,49,231,216,55,181,220,242,216,187},
	{118,133,94,121,66,167,251,123,30,51,150,17,238,214,82,80},
	{217,116,74,181,28,138,51,65,241,105,126,245,31,14,65,252},
	{93,63,132,182,96,68,111,142,106,247,15,203,217,16,232,128},
	{112,103,52,34,176,50,143,199,83,113,168,83,191,249,169,235},
	{204,14,60,247,194,172,22,162,44,43,122,32,24,15,23,109},
	{90,34,21,145,13,226,153,173,38,240,94,90,80,23,55,30},
	{252,231,179,189,105,10,210,54,157,119,71,126,98,89,113,123},
	{67,227,32,188,32,164,65,98,251,140,212,121,205,235,177,76},
	{199,72,170,122,125,37,228,41,213,89,114,113,223,248,250,216},
	{184,63,43,141,10,225,214,201,166,253,220,170,179,105,96,233},
	{103,30,98,124,48,141,152,109,132,55,155,149,75,158,107,31},
	{119,166,68,49,184,2,254,54,123,63,179,201,227,40,126,17},
	{247,136,39,234,42,68,61,183,114,238,13,142,5,168,143,106},
	{145,101,99,209,151,91,159,149,113,120,54,43,113,51,214,168},
	{217,167,152,162,33,2,27,221,63,94,132,226,172,13,181,204},
	{185,205,29,254,38,7,134,100,12,125,186,14,217,85,71,217},
	{188,107,185,239,174,153,175,69,109,57,131,77,18,154,137,92},
	{30,214,183,177,4,146,62,182,117,230,36,129,195,125,60,126},
	{204,170,129,44,12,212,135,44,93,120,69,33,229,135,72,200},
	{73,137,39,87,148,64,66,75,153,220,19,26,113,127,239,140},
	{224,236,202,120,11,37,217,215,18,105,230,139,150,248,3,68},
};

static const char * titles[] = {
	"Eastern Lunar Overworld Theme",
	"Hiro's Home",
	"*Silence*",
	"Western Lunar Overworld Theme",
	"Mystere's Theme",
	"Battle Theme",
	"Boss Theme",
	"Althena's Heroes Battle Theme",
	"Omni Zophar Battle Theme",
	"Zophar Battle Theme",
	"Final Zophar Battle Theme",
	"Master Zophar 1",
	"Master Zophar 2",
	"Master Zophar 3",
	"Master Zophar 4",
	"Master Zophar 5",
	"Master Zophar 6",
	"Master Zophar 7",
	"Master Zophar 8",
	"Sneaking on the Destiny",
	"Beneath Pentagulia",
	"Forest of Illusion",
	"Meribian Sewers",
	"Dragon Caves",
	"Zophar's Domain",
	"Blue Spire Sparkling",
	"Transmission Room Pulsating",
	"Larpa",
	"Takkar",
	"Horam",
	"Carnival",
	"Pentagulia",
	"Dalton",
	"Sad Violin Theme",
	"Sunken Tower",
	"Title Screen",
	"Game Menu",
	"Destiny Travelling",
	"Goodbye to Friends",
	"Dragon's Lair",
	"Blowing Wind",
	"Taben's Peak",
	"Eastern Lunar Overworld (midway)",
	"Hiro's House Remix",
	"Western Lunar Overworld (midway)",
	"Magic Tester Battle Theme",
	"Indoor Wind",
	"Cave of Trials",
	"Mystic Ruins",
	"Tavern Theme",
	"Carnival in Illusion Woods",
	"Giggle Tent",
	"Star Tower Theme",
	"Hiro's Introduction",
	"Dragonship Destiny Theme",
	"First Sight of the Destiny",
	"The Destiny leaves for the Blue Spire",
	"Something appears at the Blue Spire",
	"Teleport to the top of the Blue Spire",
	"Ruby sees the Transfer Crystal",
	"Ruby snaps Hiro out of it",
	"Lucia begins to glow",
	"Zophar strips away Lucia's power",
	"Riding Gwyn's boat",
	"Ronfar gets angry",
	"Lucia's Piano Theme",
	"Mauri's plague flashback",
	"Lucia singing on the Destiny",
	"Lucia speaks with Leo in Dalton",
	"Plantella's roar",
//	"*Silence*",
	"Lemina is angry at the tester's destruction",
	"Lemina congratulates the group",
	"Magic Arrow launching",
	"Nauseating flight",
	"Resonating chamber",
	"Ghaleon's appearance",
	"Ghaleon's introduction",
	"Avalanche",
	"Strange white cat",
//	"*Silence*",
	"Encounter with the masked man",
	"Lemina tells Borgan to leave Vane",
//	"*Silence*",
	"Ruby concerned about Lucia on the Destiny",
	"Ghaleon's theme 1",
	"Althena appears",
	"Lucia discovers Althena is a fake",
	"Trouble",
	"Lucia doubting Althena's plan",
	"Ghaleon talking to Lucia in Pentagulia Shrine",
	"Zophar talking to Ghaleon near Goddess Tower",
	"Nall bragging to Ruby in White Dragon Cave",
	"Nall transformed",
//	"*Silence*",
	"Jean confronts Lunn",
	"Freeing the Blue Dragon",
	"NeoVane Floating",
	"NeoVane paralyzes the Destiny",
	"Ronfar and Jean get dropped",
	"Lemina confronts Borgan",
	"NeoVane falls",
//	"Lucia's Piano Theme",
	"Freeing the Black Dragon",
	"Mauri's Judgement",
	"Ruby's empowerment",
	"Ghaleon's crisis",
	"Zophar music",
	"Lucia summons the dragons",
	"Althena panics",
	"Meeting Nall in the Goddess Tower",
	"Lucia discovers Luna's recording",
	"Lucia is abandoned",
	"Rumbling",
	"Water disappears",
//	"Vane after the disaster",
	"Pentagulia dock",
	"Alarm",
	"Leo joins the group in Vane",
	"Ghaleon on the roof",
	"Ghaleon's victory",
	"Humanity shines through",
	"Ghaleon's death scream",
	"Ghaleon's redemption",
//	"*Silence*",
	"Destiny charges to Zophar's Domain",
//	"*Silence*",
	"Lucia's prison",
//	"*Silence*",
	"Finding Lucia",
	"Zophar revives",
	"Zophar Battle Recovery",
	"Omni Zophar defeated",
	"Zophar's death",
	"Hiro's victory",
	"Vane after the victory",
	"Hiro leaves Vane",
	"Lucia's theme",
	"Lucia's farewell",
	"Lucia's pendant",
	"Hiro's determination",
//	"*Silence*",
	"Zen Zone",
	"Lemina's Theme",
	"Destiny idling",
	"Plantella kidnapping",
	"Althena singing",
	"Zophar music",
	"Ghaleon explaining his plot",
	"Azado monster summoning by priest",
	"Azado monster summoning by Ronfar",
	"Repelled from the Goddess Tower",
//	"*Silence*",
	"Explosion",
	"Destiny Brig",
	"Ocean waves",
	"Ghaleon's Theme 2",
	"Lucia siezes Althena's power",
	"Pentagulia crumbling",
	"Ronfar's farewell",
	"Lemina's farewell",
	"Jean's farewell",
	"Leo's farewell",
	"Ruby's farewell",
	"Lucia leaves",
	"Hiro chips away at the Dragon's Eye",
	"?",
	"Blue Star intro",
	"Opening credits",
	"Lucia realizes her mission",
	"Lunar intro",
	"Leo's introduction",
	"Lucia appears",
	"Crystal glowing",
	"Lucia sees the sky of Lunar",
	"Lucia is sick",
	"Seeing Larpa",
	"Ronfar's challenge",
	"Lucia leaves for Pentagulia",
	"Jean's alluring dance music",
	"Lemina's introduction",
	"Trying on disguises",
	"Punching out perverts",
	"Lucia's new threads",
	"Holographic recording",
	"Ghaleon appears",
	"Lunn's introduction",
	"Seeing Taben's peak",
	"Nall's introduction",
	"Lucia in the spring",
	"Borgan's introduction",
	"Lemina crying",
	"Mauri's introduction",
	"Lucia on the Destiny",
	"Approaching Pentagulia 1",
	"Ghaleon's welcome to Pentagulia",
	"Althena's introduction",
	"Ghaleon shooting down Lucia's hopes",
	"Fighting Leo",
	"White Dragon",
	"Jean's destiny",
	"Lunn at the tournament",
	"Blue Dragon",
	"Neo Vane",
	"Challenging Borgan",
	"Black Dragon",
	"Mauri in Serak Palace",
	"Red Dragon",
	"Approaching Pentagulia 2",
	"Pentagulia sky darkening",
	"Dragons destroying Pentagulia",
	"Luna's Introduction",
	"Luna's Story 1",
	"Zophar descending",
	"Zophar wreaking havoc",
	"Lucia confronts Zophar",
	"Zophar gloats at the top of his tower",
	"Ghaleon taunting Hiro",
	"Ghaleon's atonement",
	"Friends' confidence",
	"Ghaleon's farewell",
	"Lucia imprisoned",
	"Zophar's welcome",
	"Hiro and Lucia",
	"Lucia's farewell",
	"Ruby helps Hiro",
	"Hiro finds Lucia",
	"Luna's Story 2",
	"Lucia uses Althena's light",
	"Outtakes",
};

static const short tracks[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
	18, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
	39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
	55, 56, 57, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
	73, 74, 75, 76, 78, 79, 80, 81, 82, 83, 84, 85, 86, 88, 89, 91,
	92, 93, 94, 95, 96, 97, 98, 99, 100, 102, 103, 104, 105, 106,
	107, 108, 110, 111, 112, 113, 115, 116, 117, 118, 119, 120, 121,
	122, 125, 126, 127, 128, 129, 130, 131, 132, 134, 136, 138, 139,
	140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 151, 152, 153,
	154, 155, 156, 157, 158, 159, 160, 162, 163, 164, 165, 166, 167,
	168, 169, 170, 171, 172, 173, 174, 175, 200, 201, 202, 203, 204,
	205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
	218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230,
	231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243,
	244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256,
	257, 258, 260, 261, 263, 264,
};

class input_lunar2
{
	int size, loop_start, loop_end, channels, pos, postmp, no_loop;
	service_ptr_t<file> m_file;
	mem_block_t<unsigned char> buffer;

public:
	input_lunar2()
	{
		channels = 0;
	}

	~input_lunar2()
	{
	}

	t_io_result open( service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) return io_result_error_data;

		t_io_result status;

		if ( p_filehint.is_empty() )
		{
			status = filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );
			if ( io_result_failed( status ) ) return status;
		}
		else m_file = p_filehint;

		return io_result_success;
	}

private:
	t_io_result open_internal( abort_callback & p_abort )
	{
		if ( ! m_file->can_seek() ) return io_result_error_data;

		t_io_result status;
		unsigned char * p;

		t_filesize sz;
		status = m_file->get_size( sz, p_abort );
		if ( io_result_failed( status ) ) return status;
		if ( sz < 0 || sz > ( 1 << 30 ) ) return io_result_error_data;
		size = int( sz );

		if ( ! buffer.set_size( 8192 ) )
			return io_result_error_out_of_memory;
		
		p = buffer.get_ptr();

		status = m_file->read_object( p, 8192, p_abort );
		if ( io_result_failed( status ) ) return status;

		if ( p[ 1 ] == 1 ) channels = 2;
		else if ( p[ 1 ] == 2 ) channels = 1;
		loop_start = ( ( p[ 2 ] << 24 ) | ( p[ 3 ] << 16 ) | ( p[ 4 ] << 8 ) | p[ 5 ] ) << 11;
		loop_end = ( ( p[ 6 ] << 24 ) | ( p[ 7 ] << 16 ) | (p[ 8 ] << 8 ) | p[ 9 ] ) + 1;

		return io_result_success;
	}

public:
	t_io_result get_info( file_info & p_info, abort_callback & p_abort )
	{
		if ( ! channels )
		{
			t_io_result status = open_internal( p_abort );
			if ( io_result_failed( status ) ) return status;
		}

		hasher_md5_result digest;
		int n;

		digest = static_api_ptr_t<hasher_md5>()->process_single( buffer.get_ptr(), 8192 );

		for ( n = 0; n < 219; ++n )
		{
			if ( ! memcmp( sums[n], & digest, 16 ) )
			{
				char meh[ 16 ];
				p_info.meta_add( "title", titles[ n ] );
				itoa( tracks[n], meh, 10 );
				p_info.meta_add( "tracknumber", meh );
				break;
			}
		}

		if (n == 219) return io_result_error_data;

		p_info.meta_add( "album", "Lunar: Eternal Blue" );
		p_info.meta_add( "artist", "Isao Mizoguchi" );

		p_info.info_set_int( "samplerate", 16282 );
		p_info.info_set_int( "bitspersample", 8 );
		p_info.info_set_int( "channels", channels );
		p_info.info_set_int( "bitrate", 16282 * 2 * channels / 125 );
		p_info.info_set( "codec", "Lunar:EB PCM" );
		p_info.set_length( double( loop_end ) / 16282. );

		return io_result_success;
	}

	t_io_result get_file_stats( t_filestats & p_stats,abort_callback & p_abort )
	{
		return m_file->get_stats( p_stats, p_abort );
	}

	t_io_result decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		t_io_result status;

		if ( ! channels )
		{
			status = open_internal( p_abort );
			if ( io_result_failed( status ) ) return status;
		}

		status = m_file->seek( 2048, p_abort );
		if ( io_result_failed( status ) ) return status;

		if ( ! buffer.set_size(2048 * channels) )
			return io_result_error_out_of_memory;

		pos = postmp = 0;
		no_loop = p_flags & input_flag_no_looping;

		return io_result_success;
	}

	t_io_result decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if ( no_loop && ( pos >= loop_end ) ) return io_result_eof;

		unsigned char * p = buffer.get_ptr();

		int n, t, a;
		t_io_result status;

		if (channels == 1)
		{
			status = m_file->read_object(p, 2048, p_abort);
			if (io_result_failed(status)) return status;
			for (n=0; n<1024; n++)
			{
				t = p[n*2+1];
				a = t & 0x7f;
				p[n] = (t & 0x80) ? (128 - a) : (128 + a);
			}
		}
		else
		{
			status = m_file->read_object(p, 4096, p_abort);
			if (io_result_failed(status)) return status;
			for (n=0; n<1024; n++)
			{
				t = p[n*2+1];
				a = t & 0x7f;
				p[n*2] = (t & 0x80) ? (128 - a) : (128 + a);
				t = p[n*2+2049];
				a = t & 0x7f;
				p[n*2+1] = (t & 0x80) ? (128 - a) : (128 + a);
			}
		}

		pos += 1024;
		if (pos >= loop_end)
		{
			if (!no_loop)
			{
				pos = loop_start >> channels;
				status = m_file->seek(loop_start + 2048, p_abort);
				if (io_result_failed(status)) return status;
			}
			n = (loop_end % 1024) * channels;
		}
		else
		{
			n = 1024 * channels;
		}
		bool res;
		if (postmp)
		{
			res = p_chunk.set_data_fixedpoint( p + postmp * channels, n - postmp * channels, 16282, channels, 8, audio_chunk::g_guess_channel_config( channels ) );
			postmp = 0;
		}
		else res = p_chunk.set_data_fixedpoint( p, n, 16282, channels, 8, audio_chunk::g_guess_channel_config( channels ) );
		return res ? io_result_success : io_result_error_out_of_memory;
	}

	t_io_result decode_seek( double p_seconds, abort_callback & p_abort )
	{
		pos = int( p_seconds * 16282. );
		postmp = pos & 1023;
		pos &= ~1023;
		return m_file->seek( ( pos << channels ) + 2048, p_abort );
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort )
	{
	}

	t_io_result retag( const file_info & p_info,abort_callback & p_abort )
	{
		return io_result_error_data;
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		if ( stricmp( p_extension, "pcm" ) ) return false;
		string8 name( p_full_path + string8::g_scan_filename( p_full_path ) );
		name.truncate( name.find_last( '.' ) );
		if ( name.length() != 5 ) return false;

		const char * fnt = name.get_ptr();
		if (*fnt != 'R' && *fnt != 'r') return false;
		if (fnt[1] != 'P' && fnt[1] != 'p') return false;
		if (fnt[2] < '0' || fnt[2] > '2') return false;
		if (fnt[3] < '0' || fnt[3] > '9') return false;
		if (fnt[4] < '0' || fnt[4] > '9') return false;
		return true;
	}
};

DECLARE_FILE_TYPE("Lunar 2: EB PCM files", "RP???.PCM");

static input_singletrack_factory_t<input_lunar2> g_input_lunar2_factory;

DECLARE_COMPONENT_VERSION("Lunar 2 PCM decoder", "0.5", "Plays the RP*.PCM files found on\nthe original Lunar: Eternal Blue CD.");
