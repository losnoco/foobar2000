typedef double ssrc_sample;

struct ssrc_config
{
	unsigned sfrq,dfrq,nch;
	bool fast;

	inline ssrc_config(unsigned p_sfrq,unsigned p_dfrq,unsigned p_nch,bool p_fast)
		: sfrq(p_sfrq), dfrq(p_dfrq), nch(p_nch), fast(p_fast)
	{
	}	
};


class ssrc_resampler
{
protected:
	ssrc_resampler() {}
public:	
	virtual ~ssrc_resampler() {}

	virtual void Write(const ssrc_sample* ptr,unsigned size) = 0;
	virtual void Finish() = 0;
	virtual unsigned GetDataDoneSize() = 0;
	virtual const ssrc_sample * GetDataDoneBuffer() = 0;
	virtual void ResetDataDone() = 0;
	virtual double GetLatency() = 0;//returns amount of audio data in in/out buffers in seconds

	static ssrc_resampler * Create(const ssrc_config & c);
	static bool CanResample(unsigned srate_from,unsigned srate_to);
};

#define SSRC_create(sfrq,dfrq,nch,fast) \
	ssrc_resampler::Create(ssrc_config(sfrq,dfrq,nch,fast))


/*
usage:

1) Create() with your config - may return null if src/dst sample rate combo isnt supported
2) repeat:
 Write();
 GetDataDoneSize() / GetDataDoneBuffer() to get resampled data
 ResetDataDone();
3) optionally (if you are resampling a complete track rather than aborting):
 Finish(), GetDataDoneSize() / GetDataDoneBuffer() to retrieve flushed data
4) delete

*/