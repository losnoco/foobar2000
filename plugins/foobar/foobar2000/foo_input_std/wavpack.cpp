#include <input.h>
#include <tagread.h>

#include "wavpack/wavpack.h"

#define MAX_NCH 2
#define MAX_BPS 32

#define BPS (WavHdr.BitsPerSample > 16 ? 32 : 16)
#define NCH (WavHdr.NumChannels)

class input_wavpack : public input_pcm
{
private:
	struct index_point {
		int saved, sample_index;
		char unpack_data [768];
	} index_points [256];

	char sample_buffer[576*MAX_NCH*(MAX_BPS/8)*2];	// sample buffer

	int decode_pos_sample;	// current decoding position, in samples

	WaveHeader WavHdr;	// standard wav header for sample rate, etc.
	WavpackHeader WvpkHdr;	// WavPack header for version, mode, etc.


	reader* input_file;
	reader* wvc_file;
//	mem_block buffer;

	WavpackContext wpc;
	int unpack_wavpack_data (char *buffer, WavpackHeader *wphdr,int *sample_index, int sample_count, int *bytes_used);
	int wavpack_seek (WavpackHeader *wphdr, int *sample_index, int desired_index);

	int open_wavpack_file (const char *fn, WaveHeader *wavhdr, WavpackHeader *wvpkhdr, char *error);

	void kill_wavpack_index (void);

public:

	virtual int test_filename(const char * fn,const char * ext) 
	{
		return !stricmp(ext,"WV");
	}

	input_wavpack()
	{
		input_file = 0;
		wvc_file = 0;
		memset(&wpc,0,sizeof(wpc));

		memset(&WavHdr,0,sizeof(WavHdr));
		memset(&WvpkHdr,0,sizeof(WvpkHdr));

		
		
	}

	~input_wavpack()
	{
		if (input_file != 0) 
		{
			if (wpc.inbits.file == input_file)
				bs_close_read (&wpc.inbits);

//			input_file->reader_release();
		}

		if (wvc_file != 0) 
		{
			if (wpc.in2bits.file == wvc_file)
				bs_close_read (&wpc.in2bits);

			wvc_file->reader_release();
		}

		input_file = 0;
		wvc_file = 0;
		kill_wavpack_index();
	}

	virtual int open(reader * r,file_info * info,int full_open)
	{
		if (!r->can_seek()) return 0;
		input_file = r;
		char error [128];

		if (!open_wavpack_file (info->get_file_path(), &WavHdr, &WvpkHdr, error))
			return 0;

		tag_reader::process_file(r,info);

        double dur = (double)WvpkHdr.total_samples / (double)WavHdr.SampleRate;
		info->set_length(dur);
		info->info_set_int("samplerate",WavHdr.SampleRate);
		info->info_set_int("channels",WavHdr.NumChannels);
		info->info_set_int("bitspersample",WavHdr.BitsPerSample);
		info->info_set("codec","wavpack");
        if (dur > 0.) {
            __int64 size = r->get_length();
            if (wvc_file) size += wvc_file->get_length();
            info->info_set_int("bitrate", (__int64)(((double)size * 8. / dur + 500.) / 1000.));
        }

        {
            char temp[256] = "";
            if (WvpkHdr.bits) {
                if (WvpkHdr.flags & NEW_HIGH_FLAG)
                    sprintf (temp, "hybrid %s%s", ((WvpkHdr.flags & WVC_FLAG) ? "lossless" : "lossy"), ((WvpkHdr.flags & EXTREME_DECORR) ? " (high)" : ""));
                else if (!(WvpkHdr.flags & (HIGH_FLAG | FAST_FLAG)))
                    sprintf (temp, "%d-bit lossy", WvpkHdr.bits + 3);
                else
                    sprintf (temp, "%d-bit lossy (%s mode)", WvpkHdr.bits + 3, ((WvpkHdr.flags & HIGH_FLAG) ? "high" : "fast"));
                info->info_set("extrainfo","hybrid");
            }
            else {
                if (!(WvpkHdr.flags & HIGH_FLAG))
                    sprintf (temp, "lossless (fast mode)");
                else if (WvpkHdr.flags & EXTREME_DECORR)
                    sprintf (temp, "lossless (high mode)");
                else
                    sprintf (temp, "lossless");
            }
            info->info_set("compression", temp);
        }

		decode_pos_sample = 0;
		return 1;
	}

	virtual int get_samples_pcm(void ** buffer,int * size,int * srate,int * bps,int * nch)
	{
		int used_bytes = 0;


	    int tsamples = unpack_wavpack_data (sample_buffer, &WvpkHdr, &decode_pos_sample, 576, &used_bytes) * NCH;
	    int tbytes = tsamples * (BPS/8);

	    if (!tsamples)
			return 0;
		// If we are handling 32-bit data, we must convert from the
		// 24-bit native format used by WavPack.

		if (BPS == 32) {
		    int sc = tsamples;
		    char *dst, *src;

		    dst = sample_buffer + (sc * 4);
		    src = sample_buffer + (sc * 3);

		    while (sc--) {
			 *--dst = *--src;
			 *--dst = *--src;
			 *--dst = *--src;
			 *--dst = 0;
		    }
		}

		*buffer = sample_buffer;
		*size = tbytes;
		*srate = WavHdr.SampleRate;
		*bps = BPS;
		*nch = NCH;

		return 1;
	}

	virtual int seek(double seconds)
	{
		__int64 dest_sample = (__int64)((double)WavHdr.SampleRate * seconds); 

		int used_bytes = 0;

	    wavpack_seek (&WvpkHdr, &decode_pos_sample, (int)dest_sample);
	    
	    while(decode_pos_sample < dest_sample)
		{
			int tsamples = unpack_wavpack_data (sample_buffer, &WvpkHdr, &decode_pos_sample, 576, &used_bytes) * NCH;

			if (!tsamples)
				break;
	    }
		return 1;
	}

	virtual int can_seek() {return 1;}

	virtual int set_info(reader *r,const file_info * info)
	{
		return tag_writer::process_file(r,info);;
	}

};

static service_factory_t<input,input_wavpack> foo;
//////////////////////////////////////////////////////////////////////////////
// This function attempts to open the specified WavPack file. The caller    //
// must provide allocated wavefile, WavPack, and ID3 structures that will   //
// be filled in with all appropriate information (assuming the process does //
// not fail). If the function fails then an appropriate message for the     //
// user is stored at "error" and INVALID_HANDLE_VALUE is returned, other-   //
// wise the HANDLE for the file is returned (and the file is prepared for   //
// playback in case that is desired). If the "in2file" handle pointer is    //
// not NULL and the specified file could have a "correction" file connected //
// with it, an attempt is made to open the same name with a .wvc extension. //
// If this is successful, then that file's handle is stored at "in2file",   //
// otherwise INVALID_HANDLE_VALUE is stored.                                //
//////////////////////////////////////////////////////////////////////////////

int input_wavpack::open_wavpack_file (const char *fn, WaveHeader *wavhdr, WavpackHeader *wvpkhdr, char *error)
{
    long total_samples, total_header_bytes;
    RiffChunkHeader RiffChunkHeader;
    ChunkHeader ChunkHeader;

	if (input_file==0) return 0;

    if (input_file->read(&RiffChunkHeader, sizeof (RiffChunkHeader)) == sizeof (RiffChunkHeader) &&
	!strncmp (RiffChunkHeader.ckID, "RIFF", 4) && !strncmp (RiffChunkHeader.formType, "WAVE", 4)) {

	    while (1) {

		if (input_file->read(&ChunkHeader, sizeof (ChunkHeader)) != sizeof (ChunkHeader)) {
		    strcpy (error, "Not A Valid WavPack File!");
//		    input_file->reader_release();
		    return 0;
		}

		if (!strncmp (ChunkHeader.ckID, "fmt ", 4)) {

		    if (ChunkHeader.ckSize < sizeof (WaveHeader) ||
			input_file->read(wavhdr, sizeof (WaveHeader)) != sizeof (WaveHeader)) {
			    strcpy (error, "Not A Valid WavPack File!");
//			    input_file->reader_release();
			    return 0;
		    }

		    if (ChunkHeader.ckSize > sizeof (WaveHeader))
				input_file->seek2((ChunkHeader.ckSize + 1 - sizeof (WaveHeader)) & ~1L, FILE_CURRENT);
		}
		else if (!strncmp (ChunkHeader.ckID, "data", 4)) {
		    total_samples = ChunkHeader.ckSize / wavhdr->NumChannels / 2;
		    break;
		}
		else
		    input_file->seek2((ChunkHeader.ckSize + 1) & ~1L, FILE_CURRENT);
	    }

	    total_header_bytes = (long)input_file->get_position();
    }
    else {
	strcpy (error, "Can't Handle Raw WavPack Files!");
//	input_file->reader_release();
	return 0;
    }

    if (input_file->read(wvpkhdr, sizeof (WavpackHeader)) != sizeof (WavpackHeader) ||
	strncmp (wvpkhdr->ckID, "wvpk", 4)) {
	    strcpy (error, "Not A Valid WavPack File!");
//	    input_file->reader_release();
	    return 0;
    }

    if (wvpkhdr->version < 1 || wvpkhdr->version > 3) {
	strcpy (error, "Incompatible WavPack File!");
//	input_file->reader_release();
	return 0;
    }

    if (wvpkhdr->version == 3) {

	if (wvpkhdr->flags & EXTREME_DECORR) {

	    if ((wvpkhdr->flags & NOT_STORED_FLAGS) ||
		((wvpkhdr->bits) &&
		(((wvpkhdr->flags & NEW_HIGH_FLAG) &&
		(wvpkhdr->flags & (FAST_FLAG | HIGH_FLAG))) ||
		(wvpkhdr->flags & CROSS_DECORR)))) {
		    strcpy (error, "Incompatible WavPack File!");
//		    input_file->reader_release();
		    return 0;
	    }

	    if (wvpkhdr->flags & CANCEL_EXTREME)
		wvpkhdr->flags &= ~(EXTREME_DECORR | CANCEL_EXTREME);
	}
	else
	    wvpkhdr->flags &= ~CROSS_DECORR;
    }

    input_file->seek2(total_header_bytes,FILE_BEGIN);

    switch (wvpkhdr->version) {

	case 1:
	    wvpkhdr->bits = 0;
	    input_file->seek2(-2, FILE_CURRENT);

	case 2:
	    wvpkhdr->total_samples = total_samples;
	    wvpkhdr->flags = wavhdr->NumChannels == 1 ? MONO_FLAG : 0;
	    wvpkhdr->shift = 16 - wavhdr->BitsPerSample;
	    input_file->seek2(12, FILE_CURRENT);
	    break;

	case 3:
		input_file->seek2(sizeof (WavpackHeader), FILE_CURRENT);
	    break;
    }

    if (wvpkhdr->version == 3 && wvpkhdr->bits && (wvpkhdr->flags & NEW_HIGH_FLAG)) {
		string8 in2fn = fn;
		in2fn+="c";

		wvc_file = file::g_open(in2fn,reader::MODE_READ);

    }
    else wvc_file = 0;

    if (wvc_file != 0)
	wvpkhdr->flags |= WVC_FLAG;
    else
	wvpkhdr->flags &= ~WVC_FLAG;

    return 1;
}

//////////////////////////////////////////////////////////////////////////////
// Force any existing index points to be discarded. This should be done if  //
// a new file is to be played.                                              //
//////////////////////////////////////////////////////////////////////////////

void input_wavpack::kill_wavpack_index (void)
{
    memset (index_points, 0, sizeof (index_points));
}


//////////////////////////////////////////////////////////////////////////////
// Unpack the specified number of audio samples from the specified WavPack  //
// file(s) into the specified buffer. The "sample_index" value must be 0    //
// the first time this routine is called for a given file and is updated by //
// this routine according to the number of samples unpacked. This value     //
// should NOT be changed by the caller; use wavpack_seek() instead to index //
// within the file. The "bytes_used" value is not precise (and sometimes    //
// will not get written at all) but is just used for bitrate displays. The  //
// return value is the number of samples stored which may be from 16 to 48  //
// bits each (for 16-bit mono to 24-bit stereo).                            //
//////////////////////////////////////////////////////////////////////////////

int input_wavpack::unpack_wavpack_data (char *buffer, WavpackHeader *wphdr,
    int *sample_index, int sample_count, int *bytes_used)
{
    int points_index = *sample_index / ((wphdr->total_samples >> 8) + 1);
    unsigned char *start_pos, *start_pos2;
    int result = 0;

    if (!wphdr->total_samples)
	return result;

    if (!*sample_index) {

	wpc.wphdr = wphdr;
	wpc.inbits.bufsiz = wpc.in2bits.bufsiz = 512 * 1024;

	if (bs_open_read (&wpc.inbits, input_file))
	    return result;

	if ((wphdr->flags & WVC_FLAG) && bs_open_read (&wpc.in2bits, wvc_file))
	    return result;

	unpack_init (&wpc);
    }

    if (!index_points [points_index].saved) {
	index_points [points_index].sample_index = *sample_index;
	unpack_save (&wpc, index_points [points_index].unpack_data);
	index_points [points_index].saved = TRUE;
    }

    // In a perfect world we would not be poking our nose into these structures
    // to find out how much data is being used. Again, however...

    start_pos = wpc.inbits.ptr;
    start_pos2 = wpc.in2bits.ptr;

    if (sample_count > wphdr->total_samples - *sample_index)
	sample_count = wphdr->total_samples - *sample_index;

    result = unpack_samples (&wpc, buffer, sample_count);

    if (bytes_used) {
	if (wphdr->flags & WVC_FLAG) {
	    if (wpc.inbits.ptr > start_pos && wpc.in2bits.ptr > start_pos2)
		*bytes_used = (wpc.inbits.ptr - start_pos) + (wpc.in2bits.ptr - start_pos2);
	}
	else if (wpc.inbits.ptr > start_pos)
	    *bytes_used = wpc.inbits.ptr - start_pos;
    }
	
    if ((*sample_index += result) == wphdr->total_samples) {
#if 0
	if (crc != ((wphdr->flags & WVC_FLAG) ? wphdr->crc2 : wphdr->crc))
	    MessageBox (NULL, "CRC Error!", "WavPack Player", MB_OK);
#if 0
	else
	    MessageBox (NULL, "CRC Correct", "WavPack Player", MB_OK);
#endif
#endif
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////////
// Attempt to seek to the desired location. If the specified location has   //
// not been stored in the index_points array, then this routine will fail   //
// and return FALSE. In this case the caller will simply have to manually   //
// seek to the desired position by unpacking all intervening data. If the   //
// routine does not fail then the "sample_index" value is set to the actual //
// position exactly seeked to (which will be <= to the desired position)    //
// and then it will be possible to unpack from that position.               //
//////////////////////////////////////////////////////////////////////////////

int input_wavpack::wavpack_seek (WavpackHeader *wphdr, int *sample_index, int desired_index)
{
    int points_index = desired_index / ((wphdr->total_samples >> 8) + 1);

    while (points_index)
	if (index_points [points_index].saved &&
	    index_points [points_index].sample_index <= desired_index)
	        break;
	else
	    points_index--;

    if (!index_points [points_index].saved)
	return FALSE;

    if (index_points [points_index].sample_index <= *sample_index &&
	*sample_index <= desired_index)
	    return FALSE;

    *sample_index = index_points [points_index].sample_index;
    unpack_restore (&wpc, index_points [points_index].unpack_data, TRUE);
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// Close the specified WavPack file(s) and release any resources used. It's //
// a little kludgy, but we compare the file handles to make sure that we    //
// don't free the bitstream buffers that we might currently be using to     //
// play another file. Okay, it's really kludgy.                             //
//////////////////////////////////////////////////////////////////////////////


int DoReadFile (reader* hFile, void *lpBuffer, ulong nNumberOfBytesToRead,
    ulong *lpNumberOfBytesRead, ...)
{
	*lpNumberOfBytesRead = hFile->read(lpBuffer,nNumberOfBytesToRead);
	return 1;
}
