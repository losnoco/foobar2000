#include <stdio.h>
#include <string.h>

//#include <assert.h>

#ifdef _DEBUG

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern const char * g_filename;

void assert ( bool is_true, const char * reason, int value, int max )
{
	if ( ! is_true )
	{
		char foo [ 1024 ];
		_snprintf( foo, 1023, "- Assertion Failure -\n  file: %s\n  reason: %s\n  value: %u (max: %u)\n", g_filename, reason, value, max );
		foo[ 1023 ] = 0;
		OutputDebugStringA( foo );
	}
}

#endif

#include "ahx.h"
//#include "Debug.h"

// AHXSong /////////////////////////////////////////////////////////////////////////////////////////////////

AHXSong::AHXSong()
{
	Restart = PositionNr = TrackLength = TrackNr = InstrumentNr = SubsongNr = 0;
	Name = NULL;
	Positions = NULL;
	Tracks = NULL;
	Instruments = NULL;
	Subsongs = NULL;
}

AHXSong::~AHXSong()
{
	if (Name)      delete Name;
	if (Positions) delete[] Positions;
	if (Tracks) {
	               for (int i = 0, j = TrackNr + 1; i < j; i++) delete[] Tracks[i];
	               delete[] Tracks;
	}
	if (Instruments) {
	               for (int i = 1, j = InstrumentNr + 1; i < j; i++)
				   {
					   delete[] Instruments[i].Name;
					   delete[] Instruments[i].PList.Entries;
				   }
	               delete[] Instruments;
	}
	if (Subsongs)  delete[] Subsongs;
}

// AHXPlayer ///////////////////////////////////////////////////////////////////////////////////////////////

int AHXPlayer::VibratoTable[] = { 
	0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
	253,250,244,235,224,212,197,180,161,141,120,97,74,49,24,
	0,-24,-49,-74,-97,-120,-141,-161,-180,-197,-212,-224,-235,-244,-250,-253,-255,
	-253,-250,-244,-235,-224,-212,-197,-180,-161,-141,-120,-97,-74,-49,-24
};

int AHXPlayer::PeriodTable[] = {
	0x0000, 0x0D60, 0x0CA0, 0x0BE8, 0x0B40, 0x0A98, 0x0A00, 0x0970,
	0x08E8, 0x0868, 0x07F0, 0x0780, 0x0714, 0x06B0, 0x0650, 0x05F4,
	0x05A0, 0x054C, 0x0500, 0x04B8, 0x0474, 0x0434, 0x03F8, 0x03C0,
	0x038A, 0x0358, 0x0328, 0x02FA, 0x02D0, 0x02A6, 0x0280, 0x025C,
	0x023A, 0x021A, 0x01FC, 0x01E0, 0x01C5, 0x01AC, 0x0194, 0x017D,
	0x0168, 0x0153, 0x0140, 0x012E, 0x011D, 0x010D, 0x00FE, 0x00F0,
	0x00E2, 0x00D6, 0x00CA, 0x00BE, 0x00B4, 0x00AA, 0x00A0, 0x0097,
	0x008F, 0x0087, 0x007F, 0x0078, 0x0071
};
/*
char *AHXPlayer::Notes[] = {
	"---",
	"C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
	"C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
	"C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
	"C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
	"C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5"
};
*/
#ifdef __windows__
#pragma warning(disable:4309 4305)
#endif

#if 0
static const unsigned char _WhiteNoiseBig[] = {
	0x7f,0x7f,0xa8,0xe2,0x78,0x3e,0x2c,0x92,0x52,0xd5,0x80,0x80,0xab,0x80,0x7f,0x37,
	0x7f,0x7f,0x15,0x3b,0xbc,0x66,0xf3,0x7f,0x80,0x80,0x80,0x80,0x42,0xe5,0xf8,0x80,
	0x7f,0x7f,0x26,0x7f,0x80,0x97,0x80,0x5f,0xa7,0x7f,0x80,0x80,0x80,0x7f,0x7f,0x7f,
	0xce,0x79,0x8c,0x80,0x4a,0x7f,0x80,0x16,0x7f,0x7f,0x80,0x80,0x09,0xf1,0x80,0x95,
	0x78,0x78,0x7f,0xb8,0xe2,0x52,0x7f,0x08,0x93,0x7f,0x7f,0x80,0xfb,0xa8,0x44,0xe5,
	0xca,0x09,0x7f,0x80,0x7f,0x80,0xcb,0x80,0x7f,0xf7,0x80,0x80,0xb7,0x7f,0x5b,0x80,
	0x3b,0x14,0xcf,0x80,0x7f,0x80,0x16,0x1f,0x67,0xa1,0x62,0x71,0x71,0xa7,0x7f,0x44,
	0x41,0x80,0x7f,0xcd,0x41,0x43,0x4b,0xf3,0x80,0xc7,0xdf,0xdf,0xd5,0x27,0x1f,0x1f,
	0x9f,0x36,0x24,0x73,0x71,0x7f,0x80,0x7f,0x79,0x42,0x7f,0x7f,0x80,0x80,0x80,0x2e,
	0x22,0x7f,0xf2,0x46,0x80,0x80,0xb4,0xd2,0x35,0x2e,0x80,0x8f,0xb5,0xbc,0x80,0x38,
	0xf2,0x7f,0x10,0x2d,0x7f,0x7f,0x26,0x91,0x7f,0xf0,0x7f,0xdf,0x2b,0x7f,0x80,0x3e,
	0x7f,0x7f,0x80,0x80,0xab,0xae,0x7f,0xca,0x80,0x80,0xf3,0xba,0x34,0x80,0x80,0x7f,
	0x7f,0x80,0x3e,0x66,0x80,0x17,0x80,0xab,0x80,0x09,0xf3,0x7f,0x29,0x80,0xc4,0x7f,
	0x80,0xd3,0x7f,0xba,0x80,0x7f,0x80,0x9d,0x7f,0x80,0x38,0x80,0x7f,0x7f,0x7f,0x69,
	0x7f,0x7f,0x15,0x4f,0x80,0x7c,0x8c,0x1b,0x7f,0x7f,0x80,0x80,0x70,0x2b,0x80,0x7f,
	0x5a,0xc1,0x7f,0x80,0x7f,0x45,0xbb,0x80,0x7f,0xf7,0xce,0x80,0x80,0x80,0xda,0x9d,
	0x7f,0x80,0x7f,0xba,0xe2,0x02,0x80,0x95,0xba,0x80,0xfa,0xfe,0x80,0xb4,0x80,0x80,
	0x88,0x7f,0x7f,0x12,0x80,0x80,0x0e,0x9b,0x80,0x80,0x4f,0xc9,0x2b,0x80,0x77,0xb5,
	0x7f,0x51,0x7f,0x7f,0x7f,0x7f,0x80,0x7f,0xf1,0x80,0x31,0xe6,0x80,0x7f,0x80,0xa5,
	0x80,0x7f,0xca,0x7f,0x25,0x80,0x92,0xb4,0x7f,0x80,0x97,0x7f,0x7f,0x94,0x20,0x1b,
	0x3b,0x7f,0xee,0xca,0x80,0x80,0x42,0x80,0x80,0xa3,0x80,0xc5,0xf1,0x80,0x7f,0x7f,
	0x7f,0x51,0xaf,0x7f,0x35,0x42,0x80,0x7f,0xf1,0x80,0xc5,0x7f,0x7f,0x7f,0x80,0x28,
	0x7f,0xb3,0x2c,0x2c,0xea,0x7f,0x7f,0x80,0x7f,0x21,0xa9,0x7f,0x34,0x7f,0xae,0x1e,
	0xc5,0xbf,0xae,0x7f,0x8b,0x37,0x7f,0x0d,0x80,0x73,0x23,0xbb,0x80,0x80,0xc6,0x80,
	0xb6,0x80,0x7f,0x80,0x80,0x7f,0x7f,0x80,0x21,0x7f,0x20,0x45,0xa7,0xca,0x7f,0x80,
	0x80,0x80,0x3d,0x7f,0x15,0x45,0xf3,0xd8,0x8b,0x9b,0xce,0x55,0x80,0x80,0x7f,0xbd,
	0xce,0x7f,0x36,0x80,0x7f,0xbf,0x62,0x23,0x07,0x25,0xf1,0xca,0x59,0x7f,0xaa,0x7f,
	0x7f,0x47,0x93,0x80,0x1b,0x21,0x80,0x9b,0xca,0x80,0x2d,0x80,0x98,0x7f,0x7f,0x7f,
	0xee,0x80,0x80,0x80,0x7f,0x20,0x3b,0x80,0x3c,0x22,0xcf,0x7f,0x80,0x80,0x59,0x9d,
	0x7f,0x2a,0x7f,0x80,0x7c,0x80,0xd3,0x21,0x80,0xa7,0x7f,0x7f,0x80,0x09,0x3d,0x7f,
	0x7f,0xae,0x80,0xa7,0x80,0x7f,0x73,0x05,0x3d,0x80,0x7f,0x7f,0x7f,0x26,0x3b,0x7f,
	0xf6,0x80,0x7f,0x5e,0x47,0xdf,0x80,0x7c,0x36,0x36,0x7f,0xff,0xbc,0xbc,0xbc,0x7f,
	0x7f,0x7f,0x80,0x80,0x4d,0x21,0x7f,0x7f,0x7f,0x41,0x4d,0x80,0x7f,0x7f,0x80,0xc0,
	0xaf,0x2c,0x7f,0x17,0x35,0x80,0x80,0x7f,0xf0,0x3c,0x12,0x87,0x7f,0x80,0x80,0x13,
	0x73,0x2d,0x3e,0x80,0x7f,0x80,0xa6,0xd8,0x19,0x80,0x7f,0x27,0x80,0x7f,0x80,0x7f,
	0x80,0x7f,0x23,0x80,0x4d,0x80,0x7f,0x7f,0x89,0x7f,0x80,0xb5,0x4a,0x17,0xaf,0x88,
	0x95,0x80,0x70,0x77,0x97,0x7f,0x80,0x80,0x22,0x9b,0x02,0x2f,0x80,0x80,0x98,0x7f,
	0x7f,0x12,0x2d,0x28,0xce,0xaf,0x90,0x58,0xe9,0x1a,0x71,0x2f,0x5c,0x7f,0x80,0x7f,
	0x7f,0x80,0x7f,0x47,0xcd,0xaf,0x2c,0x06,0x80,0x2f,0x80,0xe8,0x80,0x2e,0x58,0x11,
	0xd7,0xad,0x58,0x43,0x17,0x9f,0x70,0xc3,0x80,0x70,0x19,0xc3,0x37,0x2e,0x42,0x80,
	0x2c,0xbc,0x80,0x7f,0x7f,0x7f,0x10,0x45,0x2d,0x3e,0x3e,0x90,0x80,0xa6,0xd8,0x5b,
	0x80,0x7f,0x27,0x80,0x7f,0x80,0x33,0x80,0x75,0x80,0x7f,0x7f,0x94,0x80,0x21,0xf1,
	0x7f,0xee,0x7f,0xae,0xf6,0xae,0x80,0x41,0x80,0xa5,0x7f,0x40,0x7f,0x8a,0x3d,0x12,
	0xdd,0x7f,0x9e,0x7f,0x92,0x36,0x66,0x19,0x80,0x80,0xa7,0xa0,0x90,0x80,0x5f,0x23,
	0x57,0x80,0x31,0x80,0x2d,0x36,0xa0,0xd2,0x8f,0xd9,0x3f,0x80,0x3e,0x80,0x29,0xd8,
	0xad,0x7f,0x7f,0x51,0xbb,0x70,0xcb,0xb5,0xdc,0x3d,0xc2,0xb7,0x7f,0xba,0x80,0x3e,
	0x80,0x7f,0x3b,0x44,0x80,0xa6,0x7f,0x80,0x80,0x7c,0x80,0x61,0x7f,0xca,0x7f,0x7f,
	0x80,0xff,0x34,0x7f,0x46,0x05,0x7f,0x24,0x7f,0x7f,0x7f,0x7f,0xbc,0x7f,0x7f,0x7f,
	0x80,0x7f,0x15,0x7f,0xce,0xe5,0x7f,0x80,0x7f,0xbd,0x58,0x85,0x33,0x7f,0x7e,0x80,
	0x80,0x80,0x7f,0x7f,0x80,0x7f,0xf7,0x32,0x94,0x40,0x73,0x7f,0x7f,0xee,0xdc,0x7f,
	0x24,0x7f,0x7f,0xba,0xc6,0x27,0x21,0x95,0x80,0x3d,0xa4,0x80,0x7f,0x7f,0x80,0x7f,
	0x7f,0x94,0x7f,0x7f,0x94,0x80,0x61,0x7f,0x80,0x7f,0x7f,0x79,0x80,0x42,0x7f,0xbe,
	0x80,0x80,0xc2,0x43,0xf7,0xac,0xac,0x80,0x7f,0x7f,0x7f,0x80,0x14,0x7f,0x15,0x7f,
	0xc2,0x1d,0x7f,0x80,0x7f,0xbb,0x80,0x80,0x80,0x80,0xb6,0x7f,0x7f,0x44,0x7f,0x09,
	0x07,0x80,0x7f,0x80,0x7f,0x7f,0x96,0x7f,0xce,0x80,0x80,0x61,0x65,0x80,0x2d,0x4a,
	0x7f,0x7f,0x80,0x7f,0x46,0x80,0x7f,0xaa,0x44,0x80,0xcb,0x89,0x7f,0x80,0x7f,0x80,
	0x7f,0x8e,0x9f,0x80,0xc3,0x43,0x71,0x99,0x80,0x7f,0x47,0x41,0xaf,0x80,0x3b,0xb6,
	0x7f,0x72,0x80,0xd1,0x80,0x7f,0x44,0x80,0x2f,0x7f,0x7f,0x42,0x80,0x7f,0xf0,0x7f,
	0x45,0x7f,0x80,0x7f,0x80,0xc0,0xaf,0x7f,0x9c,0x1e,0x35,0x7f,0xca,0x65,0xf1,0x3c,
	0x92,0xb4,0xa0,0x80,0x7f,0x7f,0x0f,0xd7,0x73,0x80,0x0e,0x80,0x7f,0x80,0x7c,0xca,
	0xc7,0xad,0x80,0x80,0x3d,0x9e,0xf0,0x82,0x8d,0xd9,0x19,0x7f,0x93,0x7f,0x80,0x80,
	0x80,0x98,0x80,0x80,0x7f,0x3b,0x28,0xce,0x09,0x7f,0x5e,0xe9,0x80,0x80,0x7f,0x45,
	0x80,0xfa,0x7f,0x7f,0x80,0x7f,0x80,0x7f,0x7f,0x11,0x80,0xb4,0x2c,0x80,0x13,0x7f,
	0x80,0x80,0xc5,0x7f,0x7f,0xee,0x82,0x80,0x80,0x41,0x80,0x11,0x7f,0x80,0xc1,0x7f,
	0xad,0x7f,0x7f,0x7f,0x81,0xf1,0x80,0x31,0xa0,0x80,0x7f,0x7f,0x25,0x57,0x7f,0xc4,
	0x80,0x2d,0x36,0x7f,0xbd,0x80,0xd9,0x7f,0xbb,0x7f,0x80,0x2f,0x7f,0x36,0x80,0x3e,
	0x58,0x80,0x80,0x41,0x5f,0x80,0x22,0x80,0x80,0xcc,0x7f,0x7f,0x24,0xc5,0x29,0xe6,
	0xc4,0x7f,0x80,0xd1,0x80,0x3a,0x0c,0xa1,0x80,0xb7,0x7f,0xbe,0x80,0x14,0x95,0x80,
	0xf3,0x7f,0x89,0x80,0xc1,0x7f,0x80,0x7f,0x7f,0xa8,0x1e,0xc3,0x43,0x21,0x80,0x80,
	0x7f,0x47,0xcd,0x7b,0x80,0x3b,0x80,0x7f,0x25,0x80,0xd1,0x27,0x89,0x7f,0x80,0x28,
	0xa4,0x90,0x7f,0x59,0x7f,0x24,0x7f,0xb1,0x5c,0x7f,0xbf,0x7f,0x7f,0x80,0x16,0x80,
	0xdb,0x80,0x7f,0x80,0x7f,0x7f,0xf5,0xb2,0x7f,0x7f,0x80,0x7f,0x0f,0x80,0x80,0x80,
	0x77,0x80,0x2e,0x80,0x3c,0xa0,0x7f,0x2b,0x7f,0x68,0x80,0xc0,0x7f,0x7f,0x7f,0x10,
	0xb5,0x7f,0xca,0x11,0x91,0x80,0x95,0x7f,0x7f,0x7f,0x7f,0x80,0x80,0xcb,0x80,0x7f,
	0x81,0x7f,0xac,0xaa,0x7f,0x7f,0x80,0x93,0x3a,0xc0,0x80,0x80,0x98,0x52,0x80,0x7f,
	0xe1,0xa8,0xdc,0x85,0xb3,0x76,0x7f,0xba,0x80,0x7f,0xa3,0x80,0xb4,0x80,0xc6,0x21,
	0x7f,0x0f,0x7f,0x7f,0x80,0x09,0x7f,0x7f,0x7f,0xa1,0xf8,0x7f,0xa3,0x7f,0x26,0x80,
	0xc3,0x80,0x41,0x2b,0x7f,0x7f,0x80,0xc1,0x55,0x7f,0x7f,0x7f,0xaf,0x80,0x80,0x80,
	0x31,0x80,0x7f,0x7f,0xbf,0x52,0x39,0x66,0x73,0xf7,0x5c,0xe9,0x80,0x7f,0x7f,0x42,
	0x55,0x80,0x80,0x92,0x7f,0x7f,0x80,0x97,0x7f,0x15,0x80,0x23,0x1b,0xbb,0x9a,0x80,
	0x80,0x80,0xb6,0x28,0xbe,0x80,0x7f,0x0f,0xeb,0xf0,0x80,0x5f,0xc9,0x21,0x6b,0x7f,
	0x4c,0x80,0x7f,0xad,0xc4,0xc1,0x7f,0x96,0x7f,0x7f,0xaf,0x7f,0xe1,0x9e,0x80,0x7f,
	0xb3,0xf6,0x80,0x80,0x80,0x80,0xab,0xf0,0x80,0x80,0xfa,0x3a,0x7f,0x80,0x80,0x89,
	0x7f,0x08,0x7f,0x80,0x7f,0x80,0xfa,0x44,0x8f,0x09,0x7f,0x80,0x7f,0x80,0x80,0x22,
	0x9b,0x7f,0xb8,0x80,0x7f,0x7f,0x80,0x7f,0x15,0x2d,0x7f,0x7f,0x7f,0x95,0x58,0x93,
	0x7f,0xf0,0xe2,0xdc,0x7f,0x15,0x7f,0x80,0x7f,0x81,0x7f,0xf2,0x94,0x80,0x80,0x7f,
	0x80,0x7f,0xce,0x80,0x80,0x80,0x80,0x80,0x9b,0x80,0x3f,0xa2,0x80,0x98,0x02,0x7f,
	0x20,0x29,0xa8,0x78,0x7f,0x44,0x69,0x11,0x7f,0xca,0x41,0x4d,0x17,0x7f,0x7f,0x80,
	0x80,0x70,0xf7,0x7f,0xfc,0x80,0x80,0x7f,0xce,0x7f,0x80,0x80,0x4a,0x1d,0x80,0x4d,
	0x7f,0x80,0x7f,0xf2,0x80,0xfe,0x80,0x80,0xec,0x62,0x7f,0x7f,0xff,0x80,0xcb,0x80,
	0x7f,0x80,0xc0,0x7f,0x80,0x4e,0x21,0x35,0x0c,0xaf,0xb2,0x7f,0x80,0x3e,0xf0,0x96,
	0xac,0x7f,0x2b,0xea,0x80,0x80,0x80,0x80,0xa0,0x7f,0x44,0x7f,0x7f,0x6d,0xc7,0x7f,
	0x24,0x80,0x2a,0x7f,0x80,0x3c,0x80,0xec,0x7f,0x80,0xe8,0x80,0xa4,0x2a,0x3e,0x56,
	0x80,0x80,0xd3,0xdb,0xb5,0xc0,0x80,0x7f,0xaf,0x14,0x35,0x80,0x38,0x7f,0x96,0x7f,
	0x7f,0x68,0x7f,0x7f,0x41,0x7f,0x44,0x7f,0x80,0xc7,0xc7,0x80,0x80,0x80,0x14,0x80,
	0x7f,0x7f,0xdc,0x1d,0x7f,0x7f,0x7f,0xbf,0x80,0x5c,0x80,0x77,0xf7,0xc0,0xc1,0x80,
	0x23,0x59,0x80,0x80,0x7f,0xad,0xdc,0x7f,0x8a,0x89,0x7f,0xba,0x7f,0x7f,0x80,0xa9,
	0x80,0x80,0x7f,0x4b,0x91,0x7f,0x4c,0x7f,0x44,0xaf,0x7f,0x7f,0x80,0x7f,0x7f,0xb8,
	0x80,0x3c,0x7f,0x3b,0x7f,0x80,0xe8,0x80,0x7f,0x7a,0x2c,0x56,0x80,0x7f,0x80,0xe8,
	0x7f,0x7f,0x17,0x3f,0x7f,0xd8,0x05,0x73,0xdf,0x2d,0xb4,0x80,0x7f,0x95,0x80,0x8c,
	0x7f,0x7f,0xe3,0x80,0x09,0x25,0x7f,0x7f,0x7f,0x7f,0xaa,0x7f,0x15,0xc3,0xaf,0xba,
	0x80,0x80,0x2c,0xf0,0xba,0x7f,0x7f,0x68,0x7f,0x7f,0x7f,0x17,0x4f,0x85,0x80,0x80,
	0x70,0x7f,0x9b,0x62,0x2d,0x80,0x80,0x9b,0x80,0x80,0x95,0x80,0x98,0x7f,0xf7,0x7f,
	0x36,0x80,0x80,0x80,0x7f,0x27,0x80,0x7f,0xca,0x27,0x80,0x0e,0x80,0x3a,0x80,0x80,
	0x31,0xf0,0x7f,0x94,0xb2,0x52,0x7f,0x80,0x80,0x88,0x5d,0x05,0xa3,0x14,0x91,0x80,
	0xcc,0x7f,0x80,0x7f,0x7f,0x80,0x80,0x7f,0x80,0x7f,0x7f,0x4c,0x7f,0xf6,0x7f,0x7f,
	0x80,0xa4,0x7f,0x7f,0x95,0x7f,0x24,0x7f,0xf7,0x62,0x7f,0x80,0x21,0x7f,0x44,0x7f,
	0x43,0x4d,0xcb,0x80,0x7f,0x80,0xc0,0x80,0x7f,0x7f,0x12,0x35,0x24,0x4b,0x93,0x90,
	0x80,0x80,0xc7,0x2b,0x80,0x3b,0x08,0x7f,0x5e,0x7f,0x51,0x80,0xa1,0xb2,0x80,0x7f,
	0xae,0x80,0x7f,0x5a,0x4b,0xf7,0x80,0x80,0xc2,0x7f,0x80,0x80,0x92,0x34,0x80,0x95,
	0xac,0x80,0xa7,0x7f,0x7f,0x11,0x3b,0x3c,0x7f,0x80,0x7f,0x80,0xe8,0x66,0x7f,0x7f,
	0x17,0xd7,0xa3,0x3a,0x80,0x70,0x80,0x80,0x7f,0x7f,0x80,0x80,0x80,0x5c,0x2d,0x80,
	0x17,0x7f,0x7f,0x80,0x38,0x80,0xab,0x7f,0x0f,0x80,0x7f,0x80,0x80,0xc8,0xf1,0xaa,
	0x7f,0x7f,0x80,0x7f,0x7f,0x80,0x4f,0xa7,0xc4,0x80,0x02,0x37,0x80,0x3d,0x80,0x7f,
	0x7f,0xb8,0x7f,0x80,0x2f,0x14,0x13,0x80,0x38,0x80,0x7f,0xf0,0x7f,0x68,0x7f,0x59,
	0xe9,0x2a,0xce,0x7b,0x5c,0x80,0xec,0x7f,0x7f,0x7f,0xf8,0x80,0x80,0x88,0x2d,0x7f,
	0x43,0x13,0x91,0xd8,0x80,0xc4,0x7f,0x3b,0x7f,0x80,0x80,0xcb,0x80,0x80,0x80,0x7f,
	0xac,0x7f,0x26,0x7f,0x80,0x80,0xd9,0x27,0x1b,0x7f,0x7a,0x34,0x7f,0x80,0x7f,0x7f,
	0x7f,0x0c,0x7f,0x7f,0x7f,0x80,0x7f,0x80,0x17,0x80,0x6e,0x80,0x76,0x80,0x80,0x5f,
	0xa1,0xa0,0x9e,0x7f,0x4d,0x55,0xd5,0x19,0x7f,0x7f,0x7f,0x80,0x13,0xe7,0x2c,0x2c
};
#endif

AHXPlayer::AHXPlayer()
{
	OurWaves = 0;
}

AHXPlayer::~AHXPlayer()
{
	if (OurWaves) delete Waves;
}

void AHXPlayer::Init(AHXWaves* Waves /*= NULL*/)
{
	if(Waves) {
		OurWaves = 0;
		this->Waves = Waves;
	} else {
		OurWaves = 1;
		this->Waves = new AHXWaves;
	}
	WaveformTab[0] = this->Waves->Triangle04;
	WaveformTab[1] = this->Waves->Sawtooth04;
	WaveformTab[3] = this->Waves->WhiteNoiseBig;
}

int AHXPlayer::LoadSong(char* Filename)
{
	unsigned char SongBuffer[65536];
	FILE* f = fopen(Filename, "rb"); if(!f) return 0;
	int SongLength = fread(SongBuffer, 1, 65536, f);
	fclose(f);

	return LoadSong(SongBuffer, SongLength);
}

int AHXPlayer::LoadSong(void* Buffer, int Len)
{
	unsigned char* SongBuffer = (unsigned char*)Buffer;
	unsigned char* SBPtr = &SongBuffer[14];
	int SongLength = Len;
	if(SongLength < 14 || SongLength == 65536) return 0;

	if(SongBuffer[0] != 'T' && SongBuffer[1] != 'H' && SongBuffer[2] != 'X') return 0;
	Song.Revision = SongBuffer[3];
	if(Song.Revision > 1) return 0;

	// Header ////////////////////////////////////////////
	// Songname
	char* NamePtr = (char*)&SongBuffer[(SongBuffer[4]<<8) | SongBuffer[5]];
	Song.Name = new char[strlen(NamePtr)+1];
	strcpy(Song.Name, NamePtr); NamePtr += strlen(NamePtr)+1;
	Song.SpeedMultiplier = ((SongBuffer[6]>>5)&3)+1;

	Song.PositionNr = ((SongBuffer[6]&0xf)<<8) | SongBuffer[7];
	Song.Restart = (SongBuffer[8]<<8) | SongBuffer[9];
	Song.TrackLength = SongBuffer[10];
	Song.TrackNr = SongBuffer[11];
	Song.InstrumentNr = SongBuffer[12];
	Song.SubsongNr = SongBuffer[13];

	// Subsongs //////////////////////////////////////////
	Song.Subsongs = new int[Song.SubsongNr];
	for(int i = 0; i < Song.SubsongNr; i++) {
		if(SBPtr - SongBuffer > SongLength) return 0;
		Song.Subsongs[i] = (SBPtr[0]<<8)|SBPtr[1];
		SBPtr += 2;
	}

	// Position List /////////////////////////////////////
	Song.Positions = new AHXPosition[Song.PositionNr];
	for(int i = 0; i < Song.PositionNr; i++) {
		for(int j = 0; j < 4; j++) {
			if(SBPtr - SongBuffer > SongLength) return 0;
			Song.Positions[i].Track[j] = *SBPtr++;
			Song.Positions[i].Transpose[j] = *(signed char*)SBPtr++;
		}
	}

	// Tracks ////////////////////////////////////////////
	int MaxTrack = Song.TrackNr;
	Song.Tracks = new AHXStep*[MaxTrack+1];
	for(int i = 0; i < MaxTrack+1; i++) {
		Song.Tracks[i] = new AHXStep[Song.TrackLength];
		if((SongBuffer[6]&0x80)==0x80 && i==0) {
			memset(Song.Tracks[i], 0, Song.TrackLength*sizeof(AHXStep));
			continue;
		}
		for(int j = 0; j < Song.TrackLength; j++) {
			if(SBPtr - SongBuffer > SongLength) return 0;
			Song.Tracks[i][j].Note = (SBPtr[0]>>2)&0x3f;
			Song.Tracks[i][j].Instrument = ((SBPtr[0]&0x3)<<4) | (SBPtr[1]>>4);
			Song.Tracks[i][j].FX = SBPtr[1]&0xf;
			Song.Tracks[i][j].FXParam = SBPtr[2];
			SBPtr += 3;
		}
	}

	// Instruments ///////////////////////////////////////
	Song.Instruments = new AHXInstrument[Song.InstrumentNr+1];
	for(int i = 1; i < Song.InstrumentNr+1; i++) {
		Song.Instruments[i].Name = new char[strlen(NamePtr)+1];
		strcpy(Song.Instruments[i].Name, NamePtr); NamePtr += strlen(NamePtr)+1;
		if(SBPtr - SongBuffer > SongLength) return 0;
		Song.Instruments[i].Volume = SBPtr[0];
		Song.Instruments[i].FilterSpeed = ((SBPtr[1]>>3)&0x1f) | ((SBPtr[12]>>2)&0x20) | ((SBPtr[19]>>1)&0x40);
		Song.Instruments[i].WaveLength = SBPtr[1]&0x7;
		Song.Instruments[i].Envelope.aFrames = SBPtr[2];
		Song.Instruments[i].Envelope.aVolume = SBPtr[3];
		Song.Instruments[i].Envelope.dFrames = SBPtr[4]; //4
		Song.Instruments[i].Envelope.dVolume = SBPtr[5];
		Song.Instruments[i].Envelope.sFrames = SBPtr[6];
		Song.Instruments[i].Envelope.rFrames = SBPtr[7]; //7
		Song.Instruments[i].Envelope.rVolume = SBPtr[8];
		Song.Instruments[i].FilterLowerLimit = SBPtr[12]&0x7f;
		Song.Instruments[i].VibratoDelay = SBPtr[13]; //13
		Song.Instruments[i].HardCutReleaseFrames = (SBPtr[14]>>4)&7;
		Song.Instruments[i].HardCutRelease = SBPtr[14]&0x80?1:0;
		Song.Instruments[i].VibratoDepth = SBPtr[14]&0xf; //14
		Song.Instruments[i].VibratoSpeed = SBPtr[15];
		Song.Instruments[i].SquareLowerLimit = SBPtr[16];
		Song.Instruments[i].SquareUpperLimit = SBPtr[17]; //17
		Song.Instruments[i].SquareSpeed = SBPtr[18];
		Song.Instruments[i].FilterUpperLimit = SBPtr[19]&0x7f; //19
		Song.Instruments[i].PList.Speed = SBPtr[20];
		Song.Instruments[i].PList.Length= SBPtr[21];
		SBPtr += 22;
		Song.Instruments[i].PList.Entries=new AHXPListEntry[Song.Instruments[i].PList.Length];
		for(int j = 0; j < Song.Instruments[i].PList.Length; j++) {
			if(SBPtr - SongBuffer > SongLength) return 0;
			Song.Instruments[i].PList.Entries[j].FX[1] = (SBPtr[0]>>5)&7;
			Song.Instruments[i].PList.Entries[j].FX[0] = (SBPtr[0]>>2)&7;
			Song.Instruments[i].PList.Entries[j].Waveform = ((SBPtr[0]<<1)&6) | (SBPtr[1]>>7);
			Song.Instruments[i].PList.Entries[j].Fixed = (SBPtr[1]>>6)&1;
			Song.Instruments[i].PList.Entries[j].Note = SBPtr[1]&0x3f;
			Song.Instruments[i].PList.Entries[j].FXParam[0] = SBPtr[2];
			Song.Instruments[i].PList.Entries[j].FXParam[1] = SBPtr[3];
			SBPtr += 4;
		}
	}
	return 1;
}

int AHXPlayer::InitSubsong(int Nr)
{
	if(Nr > Song.SubsongNr) return 0;

	if(Nr == 0) PosNr = 0;
	       else PosNr = Song.Subsongs[Nr-1];

	PosJump = 0;
	PatternBreak = 0;
	MainVolume = 0x40;
	Playing = 1;
	NoteNr = PosJumpNote = 0;
	Tempo = 6;
	StepWaitFrames = 0;
	GetNewPosition = 1;
	SongEndReached = 0;
	TimingValue = PlayingTime = 0;

	for(int v = 0; v < 4; v++) Voices[v].Init();

	return 1;
}

void AHXPlayer::PlayIRQ()
{
	if(StepWaitFrames <= 0) {
		if(GetNewPosition) {
			int NextPos = (PosNr+1==Song.PositionNr)?0:(PosNr+1);
			for(int i = 0; i < 4; i++) {
				Voices[i].Track = Song.Positions[PosNr].Track[i];
				Voices[i].Transpose = Song.Positions[PosNr].Transpose[i];
				Voices[i].NextTrack = Song.Positions[NextPos].Track[i];
				Voices[i].NextTranspose = Song.Positions[NextPos].Transpose[i];
			}
			GetNewPosition = 0;
		}
		for(int i = 0; i < 4; i++) ProcessStep(i);
		StepWaitFrames = Tempo;
	}
	//DoFrameStuff
	for(int i = 0; i < 4; i++) ProcessFrame(i);
	PlayingTime++;
	if(Tempo > 0 && --StepWaitFrames <= 0) {
		if(!PatternBreak) {
			NoteNr++;
			if(NoteNr >= Song.TrackLength) {
				PosJump = PosNr+1;
				PosJumpNote = 0;
				PatternBreak = 1;
			}
		}
		if(PatternBreak) {
			if (PosJump <= PosNr) {
				SongEndReached = 1;
			}
			PatternBreak = 0;
			NoteNr = PosJumpNote;
			PosJumpNote = 0;
			PosNr = PosJump;
			PosJump = 0;
			if(PosNr == Song.PositionNr) {
				SongEndReached = 1;
				PosNr = Song.Restart;
			}
			GetNewPosition = 1;
		}
	}
	//RemainPosition
	for(int a = 0; a < 4; a++) SetAudio(a);
}

void AHXPlayer::NextPosition()
{
	PosNr++;
	if(PosNr == Song.PositionNr) PosNr = 0;
	StepWaitFrames = 0;
	GetNewPosition = 1;
}

void AHXPlayer::PrevPosition()
{
	PosNr--;
	if(PosNr < 0) PosNr = 0;
	StepWaitFrames = 0;
	GetNewPosition = 1;
}

void AHXPlayer::ProcessStep(int v)
{
	if(!Voices[v].TrackOn) return;
	Voices[v].VolumeSlideUp = Voices[v].VolumeSlideDown = 0;

	int Note = Song.Tracks[Song.Positions[PosNr].Track[v]][NoteNr].Note;
	int Instrument = Song.Tracks[Song.Positions[PosNr].Track[v]][NoteNr].Instrument;
	int FX = Song.Tracks[Song.Positions[PosNr].Track[v]][NoteNr].FX;
	int FXParam = Song.Tracks[Song.Positions[PosNr].Track[v]][NoteNr].FXParam;

	switch(FX) {
		case 0x0: // Position Jump HI
			if((FXParam & 0xf) > 0 && (FXParam & 0xf) <= 9)
				PosJump = FXParam & 0xf;
			break;
		case 0x5: // Volume Slide + Tone Portamento
		case 0xa: // Volume Slide
			Voices[v].VolumeSlideDown = FXParam & 0x0f;
			Voices[v].VolumeSlideUp   = FXParam >> 4;
			break;
		case 0xb: // Position Jump
			PosJump = PosJump*100 + (FXParam & 0x0f) + (FXParam >> 4)*10;
			PatternBreak = 1;
			break;
		case 0xd: // Patternbreak
			PosJump = PosNr + 1;
			PosJumpNote = (FXParam & 0x0f) + (FXParam >> 4)*10;
			if(PosJumpNote > Song.TrackLength) PosJumpNote = 0;
			PatternBreak = 1;
			break;
		case 0xe: // Enhanced commands
			switch(FXParam >> 4) {
				case 0xc: // Note Cut
					if((FXParam & 0x0f) < Tempo) {
						Voices[v].NoteCutWait = FXParam & 0x0f;
						if(Voices[v].NoteCutWait) {
							Voices[v].NoteCutOn = 1;
							Voices[v].HardCutRelease = 0;
						}
					}
					break;
				case 0xd: // Note Delay
					if(Voices[v].NoteDelayOn) {
						Voices[v].NoteDelayOn = 0;
					} else {
						if((FXParam & 0x0f) < Tempo) {
							Voices[v].NoteDelayWait = FXParam & 0x0f;
							if(Voices[v].NoteDelayWait) {
								Voices[v].NoteDelayOn = 1;
								return;
							}
						}
					}
					break;
			}
			break;
		case 0xf: // Speed
			Tempo = FXParam;
			if ( ! Tempo ) SongEndReached = 1;
			break;
	}
#ifdef _DEBUG
	assert( Instrument <= Song.InstrumentNr, "Illegal instrument number", Instrument, Song.InstrumentNr );
#endif
	if(Instrument && Instrument <= Song.InstrumentNr) {
		Voices[v].PerfSubVolume = 0x40;
		Voices[v].PeriodSlideSpeed = Voices[v].PeriodSlidePeriod = Voices[v].PeriodSlideLimit = 0;
		Voices[v].ADSRVolume = 0;
		Voices[v].Instrument = &Song.Instruments[Instrument];
		Voices[v].CalcADSR();
		//InitOnInstrument
		Voices[v].WaveLength = Voices[v].Instrument->WaveLength;
		Voices[v].NoteMaxVolume = Voices[v].Instrument->Volume;
		//InitVibrato
		Voices[v].VibratoCurrent = 0;
		Voices[v].VibratoDelay = Voices[v].Instrument->VibratoDelay;
		Voices[v].VibratoDepth = Voices[v].Instrument->VibratoDepth;
		Voices[v].VibratoSpeed = Voices[v].Instrument->VibratoSpeed;
		Voices[v].VibratoPeriod = 0;
		//InitHardCut
		Voices[v].HardCutRelease = Voices[v].Instrument->HardCutRelease;
		Voices[v].HardCut = Voices[v].Instrument->HardCutReleaseFrames;
		//InitSquare
		Voices[v].IgnoreSquare = Voices[v].SquareSlidingIn = 0;
		Voices[v].SquareWait = Voices[v].SquareOn = 0;
		int SquareLower = Voices[v].Instrument->SquareLowerLimit >> (5-Voices[v].WaveLength);
		int SquareUpper = Voices[v].Instrument->SquareUpperLimit >> (5-Voices[v].WaveLength);
		if(SquareUpper < SquareLower) { int t = SquareUpper; SquareUpper = SquareLower; SquareLower = t; }
		Voices[v].SquareUpperLimit = SquareUpper;
		Voices[v].SquareLowerLimit = SquareLower;
		//InitFilter
		Voices[v].IgnoreFilter = Voices[v].FilterWait = Voices[v].FilterOn = 0;
		Voices[v].FilterSlidingIn = 0;
		Voices[v].FilterSpeed = Voices[v].Instrument->FilterSpeed;
		int d3 = Voices[v].Instrument->FilterLowerLimit;
		int d4 = Voices[v].Instrument->FilterUpperLimit;
		if(d3 > d4) { int t = d3; d3 = d4; d4 = t; }
		Voices[v].FilterUpperLimit = d4;
		Voices[v].FilterLowerLimit = d3;
		Voices[v].FilterPos = 32;
		//Init PerfList
		Voices[v].PerfWait  =  Voices[v].PerfCurrent = 0;
		Voices[v].PerfSpeed =  Voices[v].Instrument->PList.Speed;
		Voices[v].PerfList  = &Voices[v].Instrument->PList;
	}
	//NoInstrument
	Voices[v].PeriodSlideOn = 0;

	switch(FX) {
		case 0x4: // Override filter
			break;
		case 0x9: // Set Squarewave-Offset
#ifdef _DEBUG
			assert( FXParam <= 63, "Illegal square wave offset position", FXParam, 63 );
#endif
//			if ( FXParam > 63 ) FXParam = 63;
			Voices[v].SquarePos = FXParam >> (5 - Voices[v].WaveLength);
			Voices[v].PlantSquare = 1;
			Voices[v].IgnoreSquare = 1;
			break;
		case 0x5: // Tone Portamento + Volume Slide
		case 0x3: // Tone Portamento (Period Slide Up/Down w/ Limit)
			if(FXParam != 0) Voices[v].PeriodSlideSpeed = FXParam;
			if(Note) {
				int Neue = PeriodTable[Note];
				int Alte = PeriodTable[Voices[v].TrackPeriod];
				Alte -= Neue;
				Neue = Alte + Voices[v].PeriodSlidePeriod;
				if(Neue) Voices[v].PeriodSlideLimit = -Alte;
			}
			Voices[v].PeriodSlideOn = 1;
			Voices[v].PeriodSlideWithLimit = 1;
			goto NoNote;
	}

	// Note anschlagen
	if(Note) {
		Voices[v].TrackPeriod = Note;
		Voices[v].PlantPeriod = 1;
	}
NoNote:
	switch(FX) {
		case 0x1: // Portamento up (Period slide down)
			Voices[v].PeriodSlideSpeed = -FXParam;
			Voices[v].PeriodSlideOn = 1;
			Voices[v].PeriodSlideWithLimit = 0;
			break;
		case 0x2: // Portamento down (Period slide up)
			Voices[v].PeriodSlideSpeed = FXParam;
			Voices[v].PeriodSlideOn = 1;
			Voices[v].PeriodSlideWithLimit = 0;
			break;
		case 0xc: // Volume
			if(FXParam <= 0x40) 
				Voices[v].NoteMaxVolume = FXParam;
			else {
				FXParam -= 0x50;
				if(FXParam <= 0x40)
					for(int i = 0; i < 4; i++) Voices[i].TrackMasterVolume = FXParam;
				else {
					FXParam -= 0xa0 - 0x50;
					if(FXParam <= 0x40)
						Voices[v].TrackMasterVolume = FXParam;
				}
			}
			break;
		case 0xe: // Enhanced commands
			switch(FXParam >> 4) {
				case 0x1: // Fineslide up (Period fineslide down)
					Voices[v].PeriodSlidePeriod = -(FXParam & 0x0f);
					Voices[v].PlantPeriod = 1;
					break;
				case 0x2: // Fineslide down (Period fineslide up)
					Voices[v].PeriodSlidePeriod = FXParam & 0x0f;
					Voices[v].PlantPeriod = 1;
					break;
				case 0x4: // Vibrato control
					Voices[v].VibratoDepth = FXParam & 0x0f;
					break;
				case 0xa: // Finevolume up
					Voices[v].NoteMaxVolume += FXParam & 0x0f;
					if(Voices[v].NoteMaxVolume > 0x40) Voices[v].NoteMaxVolume = 0x40;
					break;
				case 0xb: // Finevolume down
					Voices[v].NoteMaxVolume -= FXParam & 0x0f;
					if(Voices[v].NoteMaxVolume < 0) Voices[v].NoteMaxVolume = 0;
					break;
			}
			break;
	}
}

void AHXPlayer::ProcessFrame(int v)
{
	if(!Voices[v].TrackOn) return;

	if(Voices[v].NoteDelayOn) {
		if(Voices[v].NoteDelayWait <= 0) ProcessStep(v);
		                            else Voices[v].NoteDelayWait--;
	}
	if(Voices[v].HardCut) {
		int NextInstrument;
		if(NoteNr+1 < Song.TrackLength) NextInstrument = Song.Tracks[Voices[v].Track][NoteNr+1].Instrument;
						           else NextInstrument = Song.Tracks[Voices[v].NextTrack][0].Instrument;
		if(NextInstrument) {
			int d1 = Tempo - Voices[v].HardCut;
			if(d1 < 0) d1 = 0;
			if(!Voices[v].NoteCutOn) {
				Voices[v].NoteCutOn = 1;
				Voices[v].NoteCutWait = d1;
				Voices[v].HardCutReleaseF = -(d1 - Tempo);
			} else Voices[v].HardCut = 0;
		}
	}
	if(Voices[v].NoteCutOn) {
		if(Voices[v].NoteCutWait <= 0) {
			Voices[v].NoteCutOn = 0;
			if(Voices[v].HardCutRelease) {
				Voices[v].ADSR.rVolume = -(Voices[v].ADSRVolume - (Voices[v].Instrument->Envelope.rVolume << 8))/Voices[v].HardCutReleaseF;
				Voices[v].ADSR.rFrames = Voices[v].HardCutReleaseF;
				Voices[v].ADSR.aFrames = Voices[v].ADSR.dFrames = Voices[v].ADSR.sFrames = 0;
			} else Voices[v].NoteMaxVolume = 0;
		} else Voices[v].NoteCutWait--;
	}
	//adsrEnvelope
	if(Voices[v].ADSR.aFrames) {
		Voices[v].ADSRVolume += Voices[v].ADSR.aVolume; // Delta
		if(--Voices[v].ADSR.aFrames <= 0) Voices[v].ADSRVolume = Voices[v].Instrument->Envelope.aVolume << 8;
	} else if(Voices[v].ADSR.dFrames) {
		Voices[v].ADSRVolume += Voices[v].ADSR.dVolume; // Delta
		if(--Voices[v].ADSR.dFrames <= 0) Voices[v].ADSRVolume = Voices[v].Instrument->Envelope.dVolume << 8;
	} else if(Voices[v].ADSR.sFrames) {
		Voices[v].ADSR.sFrames--;
	} else if(Voices[v].ADSR.rFrames) {
		Voices[v].ADSRVolume += Voices[v].ADSR.rVolume; // Delta
		if(--Voices[v].ADSR.rFrames <= 0) Voices[v].ADSRVolume = Voices[v].Instrument->Envelope.rVolume << 8;
	}
	//VolumeSlide
	Voices[v].NoteMaxVolume = Voices[v].NoteMaxVolume + Voices[v].VolumeSlideUp - Voices[v].VolumeSlideDown;
	if(Voices[v].NoteMaxVolume < 0) Voices[v].NoteMaxVolume = 0;
	if(Voices[v].NoteMaxVolume > 0x40) Voices[v].NoteMaxVolume = 0x40;
	//Portamento
	if(Voices[v].PeriodSlideOn) {
		if(Voices[v].PeriodSlideWithLimit) {
			int d0 = Voices[v].PeriodSlidePeriod - Voices[v].PeriodSlideLimit;
			int d2 = Voices[v].PeriodSlideSpeed;
			if(d0 > 0) d2 = -d2;
			if(d0) {
				int d3 = (d0 + d2) ^ d0;
				if(d3 >= 0) d0 = Voices[v].PeriodSlidePeriod + d2;
				       else d0 = Voices[v].PeriodSlideLimit;
				Voices[v].PeriodSlidePeriod = d0;
				Voices[v].PlantPeriod = 1;
			}
		} else {
			Voices[v].PeriodSlidePeriod += Voices[v].PeriodSlideSpeed;
			Voices[v].PlantPeriod = 1;
		}
	}
	//Vibrato
	if(Voices[v].VibratoDepth) {
		if(Voices[v].VibratoDelay <= 0) {
			Voices[v].VibratoPeriod = (VibratoTable[Voices[v].VibratoCurrent] * Voices[v].VibratoDepth) >> 7;
			Voices[v].PlantPeriod = 1;
			Voices[v].VibratoCurrent = (Voices[v].VibratoCurrent + Voices[v].VibratoSpeed) & 0x3f;
		} else Voices[v].VibratoDelay--;
	}
	//PList
	if(Voices[v].Instrument && Voices[v].PerfCurrent < Voices[v].Instrument->PList.Length) {
		if(--Voices[v].PerfWait <= 0) {
			int Cur = Voices[v].PerfCurrent++;
			Voices[v].PerfWait = Voices[v].PerfSpeed;
			if(Voices[v].PerfList->Entries[Cur].Waveform) {
				Voices[v].Waveform = Voices[v].PerfList->Entries[Cur].Waveform-1;
				Voices[v].NewWaveform = 1;
				Voices[v].PeriodPerfSlideSpeed = Voices[v].PeriodPerfSlidePeriod = 0;
			}
			//Holdwave
			Voices[v].PeriodPerfSlideOn = 0;
			for(int i = 0; i < 2; i++) PListCommandParse(v, Voices[v].PerfList->Entries[Cur].FX[i], Voices[v].PerfList->Entries[Cur].FXParam[i]);
			//GetNote
			if(Voices[v].PerfList->Entries[Cur].Note) {
				Voices[v].InstrPeriod = Voices[v].PerfList->Entries[Cur].Note;
				Voices[v].PlantPeriod = 1;
				Voices[v].FixedNote = Voices[v].PerfList->Entries[Cur].Fixed;
			}
		}
	} else {
		if(Voices[v].PerfWait) Voices[v].PerfWait--;
		                  else Voices[v].PeriodPerfSlideSpeed = 0;
	}
	//PerfPortamento
	if(Voices[v].PeriodPerfSlideOn) {
		Voices[v].PeriodPerfSlidePeriod -= Voices[v].PeriodPerfSlideSpeed;
		if(Voices[v].PeriodPerfSlidePeriod) Voices[v].PlantPeriod = 1;
	}
	if(Voices[v].Waveform == 3-1 && Voices[v].SquareOn) {
		if(--Voices[v].SquareWait <= 0) {
			int d1 = Voices[v].SquareLowerLimit;
			int	d2 = Voices[v].SquareUpperLimit;
			int d3 = Voices[v].SquarePos;
			if(Voices[v].SquareInit) {
				Voices[v].SquareInit = 0;
				if(d3 <= d1) { 
					Voices[v].SquareSlidingIn = 1;
					Voices[v].SquareSign = 1;
				} else if(d3 >= d2) {
					Voices[v].SquareSlidingIn = 1;
					Voices[v].SquareSign = -1;
				}
			}
			//NoSquareInit
			if(d1 == d3 || d2 == d3) {
				if(Voices[v].SquareSlidingIn) {
					Voices[v].SquareSlidingIn = 0;
				} else {
					Voices[v].SquareSign = -Voices[v].SquareSign;
				}
			}
			if (d3 != d1 || d3 != d2) d3 += Voices[v].SquareSign;
			Voices[v].SquarePos = d3;
			Voices[v].PlantSquare = 1;
			Voices[v].SquareWait = Voices[v].Instrument->SquareSpeed;
		}
	}
	if(Voices[v].FilterOn && --Voices[v].FilterWait <= 0) {
		int d1 = Voices[v].FilterLowerLimit;
		int d2 = Voices[v].FilterUpperLimit;
		int d3 = Voices[v].FilterPos;
		if(Voices[v].FilterInit) {
			Voices[v].FilterInit = 0;
			if(d3 <= d1) {
				Voices[v].FilterSlidingIn = 1;
				Voices[v].FilterSign = 1;
			} else if(d3 >= d2) {
				Voices[v].FilterSlidingIn = 1;
				Voices[v].FilterSign = -1;
			}
		}
		//NoFilterInit
		int FMax = (Voices[v].FilterSpeed < 3)?(5-Voices[v].FilterSpeed):1;
		for(int i = 0; i < FMax; i++) {
			if(d1 == d3 || d2 == d3) {
				if(Voices[v].FilterSlidingIn) {
					Voices[v].FilterSlidingIn = 0;
				} else {
					Voices[v].FilterSign = -Voices[v].FilterSign;
				}
			}
			if (d1 != d2 || d1 != d3) d3 += Voices[v].FilterSign;
		}
		Voices[v].FilterPos = d3;
		Voices[v].NewWaveform = 1;
		Voices[v].FilterWait = Voices[v].FilterSpeed - 3;
		if(Voices[v].FilterWait < 1) Voices[v].FilterWait = 1;
	}
	if(Voices[v].Waveform == 3-1 || Voices[v].PlantSquare) {
		//CalcSquare
		int FilterPos = Voices[v].FilterPos;
		if (FilterPos > 63) FilterPos = 63;
		else if (FilterPos < 1) FilterPos = 1;
		char* SquarePtr = &Waves->Squares[(FilterPos-0x20)*(0xfc+0xfc+0x80*0x1f+0x80+0x280*3)];
		int X = Voices[v].SquarePos << (5 - Voices[v].WaveLength);
		if(X > 0x20) {
			X = 0x40 - X;
			Voices[v].SquareReverse = 1;
		}
		//OkDownSquare
		if(--X) SquarePtr += X << 7;
		int Delta = 32 >> Voices[v].WaveLength;
		WaveformTab[2] = Voices[v].SquareTempBuffer;
		for(int i = 0; i < (1 << Voices[v].WaveLength)*4; i++) {
			Voices[v].SquareTempBuffer[i] = *SquarePtr;
			SquarePtr += Delta;
		}
		Voices[v].NewWaveform = 1;
		Voices[v].Waveform = 3-1;
		Voices[v].PlantSquare = 0;
	}
	if(Voices[v].Waveform == 4-1) Voices[v].NewWaveform = 1;

	if(Voices[v].NewWaveform) {
		char* AudioSource = WaveformTab[Voices[v].Waveform];
		if(Voices[v].Waveform != 3-1) {
			int FilterPos = Voices[v].FilterPos;
			if (FilterPos > 63) FilterPos = 63;
			else if (FilterPos < 1) FilterPos = 1;
			AudioSource += (FilterPos-0x20)*(0xfc+0xfc+0x80*0x1f+0x80+0x280*3);
		}
		if(Voices[v].Waveform < 3-1) {
			//GetWLWaveformlor2
			static const int Offsets[] = {0x00,0x04,0x04+0x08,0x04+0x08+0x10,0x04+0x08+0x10+0x20,0x04+0x08+0x10+0x20+0x40};
			AudioSource += Offsets[Voices[v].WaveLength];
		}
		if(Voices[v].Waveform == 4-1) {
			//AddRandomMoving
			AudioSource += (WNRandom % (2*0x280)) & 0x7fe;
			//GoOnRandom
			WNRandom += 2239384;
			WNRandom = ((((WNRandom >> 8) | (WNRandom << 24)) + 782323) ^ 75) - 6735;
		}
		Voices[v].AudioSource = AudioSource;
	}
	//StillHoldWaveform
	//AudioInitPeriod
	Voices[v].AudioPeriod = Voices[v].InstrPeriod;
	if(!Voices[v].FixedNote) Voices[v].AudioPeriod += Voices[v].Transpose + Voices[v].TrackPeriod-1;
	if(Voices[v].AudioPeriod > 5*12) Voices[v].AudioPeriod = 5*12;
	if(Voices[v].AudioPeriod < 0)    Voices[v].AudioPeriod = 0;
	Voices[v].AudioPeriod = PeriodTable[Voices[v].AudioPeriod];
	if(!Voices[v].FixedNote) Voices[v].AudioPeriod += Voices[v].PeriodSlidePeriod;
	Voices[v].AudioPeriod += Voices[v].PeriodPerfSlidePeriod + Voices[v].VibratoPeriod;
	if(Voices[v].AudioPeriod > 0x0d60) Voices[v].AudioPeriod = 0x0d60;
	if(Voices[v].AudioPeriod < 0x0071) Voices[v].AudioPeriod = 0x0071;
	//AudioInitVolume
	Voices[v].AudioVolume = ((((((((Voices[v].ADSRVolume >> 8) * Voices[v].NoteMaxVolume) >> 6) * Voices[v].PerfSubVolume) >> 6) * Voices[v].TrackMasterVolume) >> 6) * MainVolume) >> 6;
}

void AHXPlayer::SetAudio(int v)
{
	if(!Voices[v].TrackOn) {
		Voices[v].VoiceVolume = 0;
		return;
	}

	Voices[v].VoiceVolume = Voices[v].AudioVolume;
	if(Voices[v].PlantPeriod) {
		Voices[v].PlantPeriod = 0;
		Voices[v].VoicePeriod = Voices[v].AudioPeriod;
	}
	if(Voices[v].NewWaveform) {
		if(Voices[v].Waveform == 4-1) {
			memcpy(Voices[v].VoiceBuffer, Voices[v].AudioSource, 0x280);
		} else {
			int WaveLoops = (1 << (5-Voices[v].WaveLength))*5;
			for(int i = 0; i < WaveLoops; i++) memcpy(&Voices[v].VoiceBuffer[i*4*(1 << Voices[v].WaveLength)], Voices[v].AudioSource, 4*(1 << Voices[v].WaveLength));
		}
		Voices[v].VoiceBuffer[0x280] = Voices[v].VoiceBuffer[0];
	}
}

void AHXPlayer::PListCommandParse(int v, int FX, int FXParam)
{
	switch(FX) {
		case 0: 
			if(Song.Revision > 0 && FXParam != 0) {
				if(Voices[v].IgnoreFilter) {
					Voices[v].FilterPos = Voices[v].IgnoreFilter;
					Voices[v].IgnoreFilter = 0;
				} else {
#ifdef _DEBUG
					assert( FXParam <= 63, "Illegal filter position", FXParam, 63 );
#endif
					if ( FXParam > 63 ) FXParam = 63;
					Voices[v].FilterPos = FXParam;
				}
				Voices[v].NewWaveform = 1;
			}
			break;
		case 1:
			Voices[v].PeriodPerfSlideSpeed = FXParam;
			Voices[v].PeriodPerfSlideOn = 1;
			break;
		case 2:
			Voices[v].PeriodPerfSlideSpeed = -FXParam;
			Voices[v].PeriodPerfSlideOn = 1;
			break;
		case 3: // Init Square Modulation
			if(!Voices[v].IgnoreSquare) {
#ifdef _DEBUG
				assert( FXParam <= 63, "Illegal square modulation position", FXParam, 63 );
#endif
				if ( FXParam > 63 ) FXParam = 63;
				Voices[v].SquarePos = FXParam >> (5-Voices[v].WaveLength);
			} else Voices[v].IgnoreSquare = 0;
			break;
		case 4: // Start/Stop Modulation
			if(Song.Revision == 0 || FXParam == 0) {
				Voices[v].SquareInit = (Voices[v].SquareOn ^= 1);
				Voices[v].SquareSign = 1;
			} else {
				if(FXParam & 0x0f) {
					Voices[v].SquareInit = (Voices[v].SquareOn ^= 1);
					Voices[v].SquareSign = 1;
					if((FXParam & 0x0f) == 0x0f) Voices[v].SquareSign = -1;
				}
				if(FXParam & 0xf0) {
					Voices[v].FilterInit = (Voices[v].FilterOn ^= 1);
					Voices[v].FilterSign = 1;
					if((FXParam & 0xf0) == 0xf0) Voices[v].FilterSign = -1;
				}
			}
			break;
		case 5: // Jump to Step [xx]
			Voices[v].PerfCurrent = FXParam;
			break;
		case 6: // Set Volume
			if(FXParam > 0x40) {
				if((FXParam -= 0x50) >= 0) {
					if(FXParam <= 0x40) Voices[v].PerfSubVolume = FXParam;
					else 
						if((FXParam -= 0xa0-0x50) >= 0) 
							if(FXParam <= 0x40) Voices[v].TrackMasterVolume = FXParam;
				}
			} else Voices[v].NoteMaxVolume = FXParam;
			break;
		case 7: // set speed
			Voices[v].PerfSpeed = Voices[v].PerfWait = FXParam;
			break;
	}
}

void AHXPlayer::VoiceOnOff(int Voice, int OnOff)
{
	if(Voice < 0 || Voice > 3) return;
	Voices[Voice].TrackOn = OnOff;
}

// AHXVoice ////////////////////////////////////////////////////////////////////////////////////////////////

AHXVoice::AHXVoice()
{
	Init();
}

void AHXVoice::Init()
{
	memset(this, 0, sizeof(AHXVoice));
	memset(VoiceBuffer, 0, 0x281);
	TrackOn = 1;
	TrackMasterVolume = 0x40;
}

void AHXVoice::CalcADSR()
{
	ADSR.aFrames = Instrument->Envelope.aFrames;
	ADSR.aVolume = Instrument->Envelope.aVolume*256/ADSR.aFrames;
	ADSR.dFrames = Instrument->Envelope.dFrames;
	ADSR.dVolume = (Instrument->Envelope.dVolume-Instrument->Envelope.aVolume)*256/ADSR.dFrames;
	ADSR.sFrames = Instrument->Envelope.sFrames;
	ADSR.rFrames = Instrument->Envelope.rFrames;
	ADSR.rVolume = (Instrument->Envelope.rVolume-Instrument->Envelope.dVolume)*256/ADSR.rFrames;
}


// AHXWaves ////////////////////////////////////////////////////////////////////////////////////////////////

AHXWaves::AHXWaves()
{
	Generate();
}

void AHXWaves::Generate()
{
	GenerateSawtooth(Sawtooth04, 0x04);
	GenerateSawtooth(Sawtooth08, 0x08);
	GenerateSawtooth(Sawtooth10, 0x10);
	GenerateSawtooth(Sawtooth20, 0x20);
	GenerateSawtooth(Sawtooth40, 0x40);
	GenerateSawtooth(Sawtooth80, 0x80);
	GenerateTriangle(Triangle04, 0x04);
	GenerateTriangle(Triangle08, 0x08);
	GenerateTriangle(Triangle10, 0x10);
	GenerateTriangle(Triangle20, 0x20);
	GenerateTriangle(Triangle40, 0x40);
	GenerateTriangle(Triangle80, 0x80);
	GenerateSquare(Squares);
	GenerateWhiteNoise(WhiteNoiseBig, 0x280*3);
	GenerateFilterWaveforms(Triangle04, LowPasses, HighPasses);
}

static inline void clip(float* x)
{
	if(*x > 127.f) { *x = 127.f; return; }
	if(*x < -128.f) { *x = -128.f; return; }
}

void AHXWaves::GenerateFilterWaveforms(char* Buffer, char* Low, char* High)
{
	int LengthTable[] = { 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 3, 7, 0xf, 0x1f, 0x3f, 0x7f,
		0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
		0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
		(0x280*3)-1
	};

	for(int temp = 0, freq = 8; temp < 31; temp++, freq += 3) {
		char* a0 = Buffer;
		for(int waves = 0; waves < 6+6+0x20+1; waves++) {
			float fre = (float)freq*1.25f/100.f;
			float high, mid = 0.f, low = 0.f;
			for(int i = 0; i <= LengthTable[waves]; i++) {
				high = a0[i] - mid - low; clip(&high);
				mid += high*fre; clip(&mid);
				low += mid*fre; clip(&low);
			}
			for(int i = 0; i <= LengthTable[waves]; i++) {
				high = a0[i] - mid - low; clip(&high);
				mid += high*fre; clip(&mid);
				low += mid*fre; clip(&low);
				*Low++ = (char)low;
				*High++ = (char)high;
			}
			a0 += LengthTable[waves]+1;
		}
	}
}

void AHXWaves::GenerateTriangle(char* Buffer, int Len)
{
	int d2 = Len;
	int d5 = d2 >> 2;
	int d1 = 128/d5;
	int d4 = -(d2 >> 1);
	char* edi = Buffer;
	int eax = 0;
	for(int ecx = 0; ecx < d5; ecx++) {
		*edi++ = eax;
		eax += d1;
	}
	*edi++ = 0x7f;
	if(d5 != 1) {
		eax = 128;
		for(int ecx = 0; ecx < d5-1; ecx++) {
			eax -= d1;
			*edi++ = eax;
		}
	}
	char* esi = edi + d4;
	for(int ecx = 0; ecx < d5*2; ecx++) {
		*edi++ = *esi++;
		if(edi[-1] == 0x7f) edi[-1] = 0x80;
		               else edi[-1] = -edi[-1];
	}
}

void AHXWaves::GenerateSquare(char* Buffer)
{
	char* edi = Buffer;
	for(int ebx = 1; ebx <= 0x20; ebx++) {
		for(int ecx = 0; ecx < (0x40-ebx)*2; ecx++) *edi++ = 0x80;
		for(int ecx = 0; ecx <       ebx *2; ecx++) *edi++ = 0x7f;
	}
}

void AHXWaves::GenerateSawtooth(char* Buffer, int Len)
{
	char* edi = Buffer;
	int ebx = 256/(Len-1), eax = -128;
	for(int ecx = 0; ecx < Len; ecx++) {
		*edi++ = eax;
		eax += ebx;
	}
}

void AHXWaves::GenerateWhiteNoise(char* Buffer, int Len)
{
/*	memcpy(Buffer, _WhiteNoiseBig, Len);*/
/*	move.l	#"AYS!",d0
.\1Loop	btst	#8,d0
	beq.b	.lower
.higher	tst	d0
	bmi.b	.mi
.pl	move.b	#$7f,(a1)+
	bra.b	.weida
.mi	move.b	#$80,(a1)+
	bra.b	.weida
.lower	move.b	d0,(a1)+	;upper byte, no floating!
.weida	ror.l	#5,d0
	eor.b	#%10011010,d0
	move	d0,d1
	rol.l	#2,d0
	add	d0,d1
	eor.w	d1,d0
	ror.l	#3,d0
	dbf	d7,.\1Loop
*/
/*	__asm {
		mov edi, Buffer
		mov ecx, Len
		mov eax, 0x41595321 // AYS!
loop0:	test eax, 0x100
		je lower
		cmp ax, 0
		jl mi
		mov byte ptr [edi], 0x7f
		jmp weida
mi:		mov byte ptr [edi], 0x80
		jmp weida
lower:	mov byte ptr [edi], al
weida:	inc edi
		ror eax, 5
		xor al, 10011010b
		mov bx, ax
		rol eax, 2
		add bx, ax
		xor ax, bx
		rol eax, 3
		dec ecx
		jnz loop0
	}*/
	unsigned int noise = 0x41595321;
	unsigned short temp;
	while (Len--) {
		if ( noise & 0x100 ) {
			if ( ( ( signed short ) noise ) < 0 ) *Buffer = -128;
			else *Buffer = 127;
		} else {
			*Buffer = (char) noise;
		}
		Buffer++;
		
		noise = (noise >> 5) | (noise << 27);
		noise ^= 0x9A;
		temp = (unsigned short) noise;
		noise = (noise << 2) | (noise >> 30);
		temp += (unsigned short) noise;
		noise ^= temp;
		noise = (noise >> 3) | (noise << 29);
	}
}

// AHXOutput ///////////////////////////////////////////////////////////////////////////////////////////////

AHXOutput::AHXOutput()
{
	Player = NULL;
	MixingBuffer = NULL;
	Playing = Paused = 0;
	memset(MixingPos, 0, sizeof(MixingPos));
}

AHXOutput::~AHXOutput()
{
//	Free();
}

int AHXOutput::Init(int Frequency, int Bits, int MixLen, float Boost, int Hz)
{
	this->MixLen = MixLen;
	this->Frequency = Frequency;
	this->Bits = Bits;
	this->Hz = Hz;
	this->MixingBuffer = new int [MixLen*Frequency/Hz];
	return SetOption(AHXOF_BOOST, Boost);
}

int AHXOutput::Free()
{
	delete MixingBuffer; MixingBuffer = 0;
	return 1;
}

int AHXOutput::SetOption(int Option, int Value)
{
	switch(Option) {
		case AHXOI_OVERSAMPLING: Oversampling = Value; return 1;
		default: return 0;
	}
}

int AHXOutput::SetOption(int Option, float Value)
{
	switch(Option) {
		case AHXOF_BOOST: {
			// Initialize volume table
			for (int i = 0; i < 65; i++)
				for (int j = -128; j < 128; j++)
					VolumeTable[i][j+128] = (int)(i * j * Value) / 64;
			Boost = Value;
		} return 1;
		default: return 0;
	}
}

int AHXOutput::GetOption(int Option, int* pValue)
{
	switch(Option) {
		case AHXOI_OVERSAMPLING: *pValue = Oversampling; return 1;
		default: return 0;
	}
}

int AHXOutput::GetOption(int Option, float* pValue)
{
	switch(Option) {
		case AHXOF_BOOST: *pValue = Boost; return 1;
		default: return 0;
	}
}

#define min(a,b)  (((a) < (b)) ? (a) : (b))

void AHXOutput::MixChunk(int NrSamples, int** mb)
{
	for(int v = 0; v < 4; v++) {
		if(Player->Voices[v].VoiceVolume == 0) continue;
		float freq = Period2Freq(Player->Voices[v].VoicePeriod);
		int delta = (int)(freq * (1 << 16) / Frequency);
		int samples_to_mix = NrSamples;
		int mixpos = 0;
		while(samples_to_mix) {
			if(MixingPos[v] > (0x280 << 16)) MixingPos[v] -= 0x280 << 16;
			int thiscount = min(samples_to_mix, ((0x280 << 16)-MixingPos[v]-1) / delta + 1);
			samples_to_mix -= thiscount;
			int* VolTab = &VolumeTable[Player->Voices[v].VoiceVolume][128];
			//INNER LOOP
			if(Oversampling) {
				for(int i = 0; i < thiscount; i++) {
					int offset = MixingPos[v] >> 16;
					int sample1 = VolTab[Player->Voices[v].VoiceBuffer[offset]];
					int sample2 = VolTab[Player->Voices[v].VoiceBuffer[offset+1]];
					int frac1 = MixingPos[v] & ((1 << 16) - 1);
					int frac2 = (1 << 16) - frac1;
					(*mb)[mixpos++] += ((sample1 * frac2) + (sample2 * frac1)) >> 16;
					MixingPos[v] += delta;
				}
			} else {
				for(int i = 0; i < thiscount; i++) {
					(*mb)[mixpos++] += VolTab[Player->Voices[v].VoiceBuffer[MixingPos[v] >> 16]];
					MixingPos[v] += delta;
				}
			}
		} // while
	} // v = 0-3
	*mb += NrSamples;
}

void AHXOutput::MixBuffer()
{
	int NrSamples = Frequency / Hz / Player->Song.SpeedMultiplier;
	int* mb = MixingBuffer;
	
	memset(MixingBuffer, 0, MixLen*Frequency/Hz*sizeof(int));
	for(int f = 0; f < MixLen*Player->Song.SpeedMultiplier /* MixLen = # frames */; f++) {
		Player->PlayIRQ();
		MixChunk(NrSamples, &mb);
	} // frames
}
