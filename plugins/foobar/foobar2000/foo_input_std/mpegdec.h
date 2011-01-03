#ifndef _FOO_INPUT_STD_MPEGDEC_H_
#define _FOO_INPUT_STD_MPEGDEC_H_

class NOVTABLE mpegdec
{
protected:
	mpegdec() {}
public:
	enum decoder_status {
		DECODE_OK,DECODE_NEED_MORE,DECODE_ERROR
	};
	virtual ~mpegdec() {}
	virtual decoder_status process_frame(const void * buf,unsigned bytes,audio_sample * out,unsigned & nch,unsigned & srate,unsigned & samples_done) = 0;//out must be at least 2304 samples (1152 * stereo)
	virtual void reset() = 0;
	static mpegdec * create_mpglib();
	static mpegdec * create_acm();
	static mpegdec * create_mad();
};


#endif