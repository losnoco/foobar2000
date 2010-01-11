//#define RG_BENCHMARK
//#define RG_HIGHPREC
//#define RG_NOASM

#ifdef RG_HIGHPREC
typedef double rg_float;
#define RG_NOASM
#else
typedef float rg_float;
#endif

typedef double rg_input;//input format can be adjusted separately from internal format; left at 64bit for foobar2000 compatibility

#ifdef RG_BENCHMARK
#define rg_profiler(x) profiler(x)
#else
#define rg_profiler(x)
#endif


class rgcalc
{
protected:
	rgcalc() {}
public:

	virtual bool InitGainAnalysis(unsigned samplefreq) = 0;

	virtual bool ProcessSamples ( const rg_input * ptr, unsigned channels, unsigned samples )=0;	//ptr == contains channels*samples values in -1..1 scale (allowed to be outside range), channels are interleaved
	
	virtual rg_float GetTitleGain () = 0;
	virtual rg_float GetAlbumGain () = 0;
	virtual unsigned GetSampleRate() = 0;
	virtual ~rgcalc() {}

	static rgcalc * create();
	static bool is_supported_samplerate(unsigned freq);

};


/*
USAGE
	rgcalc::create();

	InitGainAnalysis(mysamplerate); <= will return false on unsupported rate

	if doing album gain
	{
		repeat for all files in album
		{
			decode file, pass all data chunks to ProcessSamples()
			
			GetTitleGain() to retrieve track gain value for current track
		}
		GetAlbumGain() to retrieve album gain value
	}
	else if doing track gain
	{
		decode file, pass all data chunks to ProcessSamples()
		
		GetTitleGain() to retrieve track gain value
	}

	delete the rgcalc object
*/