#ifndef __ACM_STREAM_UNPACK
#define __ACM_STREAM_UNPACK

#include <stdlib.h>

typedef int (_stdcall* FileReadFunction) (void * hFile, unsigned char* buffer, int count);

class CACMUnpacker {
private:
// File reading
	FileReadFunction read_file; // file reader function
	void * hFile; // file handle, can be anything, e.g. ptr to reader-object

	unsigned char *fileBuffPtr, *buffPos; // pointer to file buffer and current position
	int bufferSize; // size of file buffer
	int availBytes; // new (not yet processed) bytes in file buffer

// Bits
	unsigned nextBits; // new bits
	int availBits; // count of new bits

// Parameters of ACM stream
	int	packAttrs, someSize, packAttrs2, someSize2;

// Unpacking buffers
	int *decompBuff, *someBuff;
	int blocks, totBlSize;
	int valsToGo; // samples left to decompress
	int *values; // pointer to decompressed samples
	int valCnt; // count of decompressed samples

	short Amplitude_Buffer [0x10000];
	//short *Buffer_Middle = &Amplitude_Buffer[0x8000];
#define Buffer_Middle (Amplitude_Buffer + 0x8000)

// Reading routines
	unsigned char readNextPortion(); // read next block of data
	void prepareBits (int bits); // request bits
	int getBits (int bits); // request and return next bits

public:
// These functions are used to fill the buffer with the amplitude values
	int Return0 (int pass, int ind);
	int ZeroFill (int pass, int ind);
	int LinearFill (int pass, int ind);

	int k1_3bits (int pass, int ind);
	int k1_2bits (int pass, int ind);
	int t1_5bits (int pass, int ind);

	int k2_4bits (int pass, int ind);
	int k2_3bits (int pass, int ind);
	int t2_7bits (int pass, int ind);

	int k3_5bits (int pass, int ind);
	int k3_4bits (int pass, int ind);

	int k4_5bits (int pass, int ind);
	int k4_4bits (int pass, int ind);

	int t3_7bits (int pass, int ind);
private:
// Unpacking functions
	int createAmplitudeDictionary();
	void unpackValues(); // unpack current block
	int makeNewValues(); // prepare new block, then unpack it
public:
	bool init (FileReadFunction readFunc, void * fileHandle, int &channels, int &frequency, int &samples);
		// samples = count of sound samples (one sample is 16bits)

	CACMUnpacker() {
		fileBuffPtr = NULL;
		decompBuff = NULL;
		someBuff = NULL;
	}
	~CACMUnpacker() {
		if (fileBuffPtr) delete (fileBuffPtr);
		if (decompBuff) free (decompBuff);
		if (someBuff) delete (someBuff);
	}

	int readAndDecompress (int * buff, int count);
		// read sound samples from ACM stream
		// buff  - buffer to place decompressed samples
		// count - size of buffer (in samples)
		// return: count of actually decompressed bytes
};

typedef int (CACMUnpacker::* FillerProc) (int pass, int ind);

#endif