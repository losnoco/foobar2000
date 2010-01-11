#include <math.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "replaygain.h"

#ifdef RG_BENCHMARK
#include "../../pfc/pfc.h"	//for profiler()
#else
#define tabsize(x) (sizeof(x)/sizeof(*x))
#endif




typedef unsigned long    Uint32_t;
typedef signed long      Int32_t;

#define YULE_ORDER         10
#define BUTTER_ORDER        2
#define RMS_PERCENTILE      0.95        // percentile which is louder than the proposed level
#define MAX_SAMP_FREQ   192000.          // maximum allowed sample frequency [Hz]
#define RMS_WINDOW_TIME     0.050       // Time slice size [s]
#define STEPS_per_dB      100.          // Table entries per dB
#define MAX_dB            120.          // Table entries for 0...MAX_dB (normal max. values are 70...80 dB)
#define SQRTHALF            0.70710678118654752440084436210485


#define MAX_ORDER               (BUTTER_ORDER > YULE_ORDER ? BUTTER_ORDER : YULE_ORDER)
#define MAX_SAMPLES_PER_WINDOW  (size_t) (MAX_SAMP_FREQ * RMS_WINDOW_TIME + 1)      // max. Samples per Time slice
#define PINK_REF                64.82 //298640883795                              // calibration value

#ifdef _MSC_VER
#pragma warning ( disable : 4305 )
#endif


struct rg_freqinfo
{
	unsigned rate;
	rg_float BYule[YULE_ORDER+1],AYule[YULE_ORDER+1],BButter[BUTTER_ORDER+1],AButter[BUTTER_ORDER+1];
};

static rg_freqinfo freqinfos[] =	//reminder: when editing, check MAX_SAMP_FREQ above
{
	{
		48000,
		{ 0.03857599435200, -0.02160367184185, -0.00123395316851, -0.00009291677959, -0.01655260341619,  0.02161526843274, -0.02074045215285,  0.00594298065125,  0.00306428023191,  0.00012025322027,  0.00288463683916 },
		{ 1., -3.84664617118067,  7.81501653005538,-11.34170355132042, 13.05504219327545,-12.28759895145294,  9.48293806319790, -5.87257861775999,  2.75465861874613, -0.86984376593551, 0.13919314567432 },
		{ 0.98621192462708, -1.97242384925416, 0.98621192462708 },
		{ 1., -1.97223372919527, 0.97261396931306 },
	},

	{
		44100,
	    { 0.05418656406430, -0.02911007808948, -0.00848709379851, -0.00851165645469, -0.00834990904936,  0.02245293253339, -0.02596338512915,  0.01624864962975, -0.00240879051584,  0.00674613682247, -0.00187763777362 },
		{ 1., -3.47845948550071,  6.36317777566148, -8.54751527471874,  9.47693607801280, -8.81498681370155,  6.85401540936998, -4.39470996079559,  2.19611684890774, -0.75104302451432, 0.13149317958808 },
		{ 0.98500175787242, -1.97000351574484, 0.98500175787242 },
		{ 1., -1.96977855582618, 0.97022847566350 },
	},

	{
		32000,
		{ 0.15457299681924, -0.09331049056315, -0.06247880153653,  0.02163541888798, -0.05588393329856,  0.04781476674921,  0.00222312597743,  0.03174092540049, -0.01390589421898,  0.00651420667831, -0.00881362733839 },
		{ 1., -2.37898834973084,  2.84868151156327, -2.64577170229825,  2.23697657451713, -1.67148153367602,  1.00595954808547, -0.45953458054983,  0.16378164858596, -0.05032077717131, 0.02347897407020 },
		{ 0.97938932735214, -1.95877865470428, 0.97938932735214 },
		{ 1., -1.95835380975398, 0.95920349965459 },
	},

	{
		24000,
	    { 0.30296907319327, -0.22613988682123, -0.08587323730772,  0.03282930172664, -0.00915702933434, -0.02364141202522, -0.00584456039913,  0.06276101321749, -0.00000828086748,  0.00205861885564, -0.02950134983287 },
		{ 1., -1.61273165137247,  1.07977492259970, -0.25656257754070, -0.16276719120440, -0.22638893773906,  0.39120800788284, -0.22138138954925,  0.04500235387352,  0.02005851806501, 0.00302439095741 },
		{ 0.97531843204928, -1.95063686409857, 0.97531843204928 },
		{ 1., -1.95002759149878, 0.95124613669835 },
	},

	{
		22050,
		{ 0.33642304856132, -0.25572241425570, -0.11828570177555,  0.11921148675203, -0.07834489609479, -0.00469977914380, -0.00589500224440,  0.05724228140351,  0.00832043980773, -0.01635381384540, -0.01760176568150 },
		{ 1., -1.49858979367799,  0.87350271418188,  0.12205022308084, -0.80774944671438,  0.47854794562326, -0.12453458140019, -0.04067510197014,  0.08333755284107, -0.04237348025746, 0.02977207319925 },
		{ 0.97316523498161, -1.94633046996323, 0.97316523498161 },
		{ 1., -1.94561023566527, 0.94705070426118 },
	},

	{
		16000,
		{ 0.44915256608450, -0.14351757464547, -0.22784394429749, -0.01419140100551,  0.04078262797139, -0.12398163381748,  0.04097565135648,  0.10478503600251, -0.01863887810927, -0.03193428438915,  0.00541907748707 },
		{ 1., -0.62820619233671,  0.29661783706366, -0.37256372942400,  0.00213767857124, -0.42029820170918,  0.22199650564824,  0.00613424350682,  0.06747620744683,  0.05784820375801, 0.03222754072173 },
		{ 0.96454515552826, -1.92909031105652, 0.96454515552826 },
		{ 1., -1.92783286977036, 0.93034775234268 },
	},

	{
		12000,
	    { 0.56619470757641, -0.75464456939302,  0.16242137742230,  0.16744243493672, -0.18901604199609,  0.30931782841830, -0.27562961986224,  0.00647310677246,  0.08647503780351, -0.03788984554840, -0.00588215443421 },
		{ 1., -1.04800335126349,  0.29156311971249, -0.26806001042947,  0.00819999645858,  0.45054734505008, -0.33032403314006,  0.06739368333110, -0.04784254229033,  0.01639907836189, 0.01807364323573 },
		{ 0.96009142950541, -1.92018285901082, 0.96009142950541 },
		{ 1., -1.91858953033784, 0.92177618768381 },
	},
	
	{
		11025,
		{ 0.58100494960553, -0.53174909058578, -0.14289799034253,  0.17520704835522,  0.02377945217615,  0.15558449135573, -0.25344790059353,  0.01628462406333,  0.06920467763959, -0.03721611395801, -0.00749618797172 },
		{ 1., -0.51035327095184, -0.31863563325245, -0.20256413484477,  0.14728154134330,  0.38952639978999, -0.23313271880868, -0.05246019024463, -0.02505961724053,  0.02442357316099, 0.01818801111503 },
		{ 0.95856916599601, -1.91713833199203, 0.95856916599601 },
		{ 1., -1.91542108074780, 0.91885558323625 },
	},

	{
		8000,
	    { 0.53648789255105, -0.42163034350696, -0.00275953611929,  0.04267842219415, -0.10214864179676,  0.14590772289388, -0.02459864859345, -0.11202315195388, -0.04060034127000,  0.04788665548180, -0.02217936801134 },
		{ 1., -0.25049871956020, -0.43193942311114, -0.03424681017675, -0.04678328784242,  0.26408300200955,  0.15113130533216, -0.17556493366449, -0.18823009262115,  0.05477720428674, 0.04704409688120 },
	    { 0.94597685600279, -1.89195371200558, 0.94597685600279 },
	    { 1., -1.88903307939452, 0.89487434461664 },
	},

	{
		18900,
		{0.38524531015142,  -0.27682212062067,  -0.09980181488805,   0.09951486755646,  -0.08934020156622,  -0.00322369330199,  -0.00110329090689,   0.03784509844682,   0.01683906213303,  -0.01147039862572,  -0.01941767987192 },
		{1.00000000000000,  -1.29708918404534,   0.90399339674203,  -0.29613799017877,  -0.42326645916207,   0.37934887402200,  -0.37919795944938,   0.23410283284785,  -0.03892971758879,   0.00403009552351,   0.03640166626278 },
		{0.96535326815829,  -1.93070653631658,   0.96535326815829 },
		{1.00000000000000,  -1.92950577983524,   0.93190729279793 },
	},

	{
		37800,
		{0.08717879977844,  -0.01000374016172,  -0.06265852122368,  -0.01119328800950,  -0.00114279372960,   0.02081333954769,  -0.01603261863207,   0.01936763028546,   0.00760044736442,  -0.00303979112271,  -0.00075088605788 },
		{1.00000000000000,  -2.62816311472146,   3.53734535817992,  -3.81003448678921,   3.91291636730132,  -3.53518605896288,   2.71356866157873,  -1.86723311846592,   1.12075382367659,  -0.48574086886890,   0.11330544663849 },
		{0.98252400815195,  -1.96504801630391,   0.98252400815195 },
		{1.00000000000000,  -1.96474258269041,   0.96535344991740 },
	},

	{
		56000,
		{0.03144914734085,  -0.06151729206963,   0.08066788708145,  -0.09737939921516,   0.08943210803999,  -0.06989984672010,   0.04926972841044,  -0.03161257848451,   0.01456837493506,  -0.00316015108496,   0.00132807215875 },
		{1.00000000000000,  -4.87377313090032,  12.03922160140209, -20.10151118381395,  25.10388534415171, -24.29065560815903,  18.27158469090663, -10.45249552560593,   4.30319491872003,  -1.13716992070185,   0.14510733527035 },
		{0.98816995007392,  -1.97633990014784,   0.98816995007392 },
		{1.00000000000000,  -1.97619994516973,   0.97647985512594 },
	},

	{
		64000,
		{0.02613056568174,  -0.08128786488109,   0.14937282347325,  -0.21695711675126,   0.25010286673402,  -0.23162283619278,   0.17424041833052,  -0.10299599216680,   0.04258696481981,  -0.00977952936493,   0.00105325558889 },
		{1.00000000000000,  -5.73625477092119,  16.15249794355035, -29.68654912464508,  39.55706155674083, -39.82524556246253,  30.50605345013009, -17.43051772821245,   7.05154573908017,  -1.80783839720514,   0.22127840210813 },
		{0.98964101933472,  -1.97928203866944,   0.98964101933472 },
		{1.00000000000000,  -1.97917472731009,   0.97938935002880 },
	},

	{
		88200,
		{0.02667482047416,  -0.11377479336097,   0.23063167910965,  -0.30726477945593,   0.33188520686529,  -0.33862680249063,   0.31807161531340,  -0.23730796929880,   0.12273894790371,  -0.03840017967282,   0.00549673387936 },
		{1.00000000000000,  -6.31836451657302,  18.31351310801799, -31.88210014815921,  36.53792146976740, -28.23393036467559,  14.24725258227189,  -4.04670980012854,   0.18865757280515,   0.25420333563908,  -0.06012333531065 },
		{0.99247255046129,  -1.98494510092259,   0.99247255046129 },
		{1.00000000000000,  -1.98488843762335,   0.98500176422183 },
	},

	{
		96000,
		{0.00588138296683,  -0.01613559730421,   0.02184798954216,  -0.01742490405317,   0.00464635643780,   0.01117772513205,  -0.02123865824368,   0.01959354413350,  -0.01079720643523,   0.00352183686289,  -0.00063124341421 },
		{1.00000000000000,  -5.97808823642008,  16.21362507964068, -25.72923730652599,  25.40470663139513, -14.66166287771134,   2.81597484359752,   2.51447125969733,  -2.23575306985286,   0.75788151036791,  -0.10078025199029 },
		{0.99308203517541,  -1.98616407035082,   0.99308203517541 },
		{1.00000000000000,  -1.98611621154089,   0.98621192916075 },
	},

	{
		112000,
		{0.00528778718259,  -0.01893240907245,   0.03185982561867,  -0.02926260297838,   0.00715743034072,   0.01985743355827,  -0.03222614850941,   0.02565681978192,  -0.01210662313473,   0.00325436284541,  -0.00044173593001 },
		{1.00000000000000,  -6.24932108456288,  17.42344320538476, -27.86819709054896,  26.79087344681326, -13.43711081485123,  -0.66023612948173,   6.03658091814935,  -4.24926577030310,   1.40829268709186,  -0.19480852628112 },
		{0.99406737810867,  -1.98813475621734,   0.99406737810867 },
		{1.00000000000000,  -1.98809955990514,   0.98816995252954 },
	},

	{
		128000,
		{0.00553120584305,  -0.02112620545016,   0.03549076243117,  -0.03362498312306,   0.01425867248183,   0.01344686928787,  -0.03392770787836,   0.03464136459530,  -0.02039116051549,   0.00667420794705,  -0.00093763762995 },
		{1.00000000000000,  -6.14581710839925,  16.04785903675838, -22.19089131407749,  15.24756471580286,  -0.52001440400238,  -8.00488641699940,   6.60916094768855,  -2.37856022810923,   0.33106947986101,   0.00459820832036 },
		{0.99480702681278,  -1.98961405362557,   0.99480702681278 },
		{1.00000000000000,  -1.98958708647324,   0.98964102077790 },
	},

	{
		144000,
		{0.00639682359450,  -0.02556437970955,   0.04230854400938,  -0.03722462201267,   0.01718514827295,   0.00610592243009,  -0.03065965747365,   0.04345745003539,  -0.03298592681309,   0.01320937236809,  -0.00220304127757 },
		{1.00000000000000,  -6.14814623523425,  15.80002457141566, -20.78487587686937,  11.98848552310315,   3.36462015062606, -10.22419868359470,   6.65599702146473,  -1.67141861110485,  -0.05417956536718,   0.07374767867406 },
		{0.99538268958706,  -1.99076537917413,   0.99538268958706 },
		{1.00000000000000,  -1.99074405950505,   0.99078669884321 },
	},

	{
		176400,
		{0.00268568524529,  -0.00852379426080,   0.00852704191347,   0.00146116310295,  -0.00950855828762,   0.00625449515499,   0.00116183868722,  -0.00362461417136,   0.00203961000134,  -0.00050664587933,   0.00004327455427 },
		{1.00000000000000,  -5.57512782763045,  12.44291056065794, -12.87462799681221,   3.08554846961576,   6.62493459880692,  -7.07662766313248,   2.51175542736441,   0.06731510802735,  -0.24567753819213,   0.03961404162376 },
		{0.99622916581118,  -1.99245833162236,   0.99622916581118 },
		{1.00000000000000,  -1.99244411238133,   0.99247255086339 },
	},

	{
		192000,
		{0.01184742123123,  -0.04631092400086,   0.06584226961238,  -0.02165588522478,  -0.05656260778952,   0.08607493592760,  -0.03375544339786,  -0.04216579932754,   0.06416711490648,  -0.03444708260844,   0.00697275872241 },
		{1.00000000000000,  -5.24727318348167,  10.60821585192244,  -8.74127665810413,  -1.33906071371683,   8.07972882096606,  -5.46179918950847,   0.54318070652536,   0.87450969224280,  -0.34656083539754,   0.03034796843589 },
		{0.99653501465135,  -1.99307002930271,   0.99653501465135 },
		{1.00000000000000,  -1.99305802314321,   0.99308203546221 },
	}
};

#define N_SRATES tabsize(freqinfos)


#ifndef RG_NOASM
extern "C" { int _cdecl _detect_3dnow_ex(); }

static bool detect_3dnow_ex()
{
	__try
	{
		return !!_detect_3dnow_ex();
	}
	__except(1)
	{
		return false;
	}
}

#endif

class rgcalc_i : public rgcalc
{
private:

	rg_float          linprebuf [MAX_ORDER * 2];
	rg_float*         linpre;                                          // left input samples, with pre-buffer
	rg_float          lstepbuf  [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
	rg_float*         lstep;                                           // left "first step" (i.e. post first filter) samples
	rg_float          loutbuf   [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
	rg_float*         lout;                                            // left "out" (i.e. post second filter) samples
	rg_float          rinprebuf [MAX_ORDER * 2];
	rg_float*         rinpre;                                          // right input samples ...
	rg_float          rstepbuf  [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
	rg_float*         rstep;
	rg_float          routbuf   [MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
	rg_float*         rout;
	unsigned int              sampleWindow;                                    // number of samples required to reach number of milliseconds required for RMS window
	unsigned long    totsamp;
	double           lsum;
	double           rsum;
	int              first;
	Uint32_t  A [(size_t)(STEPS_per_dB * MAX_dB)];
	Uint32_t  B [(size_t)(STEPS_per_dB * MAX_dB)];

	bool ResetSampleFrequency ( unsigned samplefreq );

	rg_float * tempbuf;
	unsigned tempbuf_size;
	
	rg_float analyzeResult ( Uint32_t* Array, size_t len );



	rg_float l_AYule[YULE_ORDER+1], l_BYule[YULE_ORDER+1];
	rg_float l_AButter[BUTTER_ORDER+1], l_BButter[BUTTER_ORDER+1];


#ifndef RG_NOASM
	static void invert_tab_3dnow(rg_float * tab,unsigned count)
	{
		unsigned idx;
		for(idx=1;idx<count;idx+=2)
		{
			rg_float t = tab[idx];
			tab[idx] = tab[idx+1];
			tab[idx+1] = t;
		}
	}
#endif
	unsigned m_srate;


	bool check_tempbuf_size(unsigned val)
	{
		if (val==0) return true;
		else if (tempbuf==0)
		{
			tempbuf_size = val;
			tempbuf = (rg_float*)malloc(val * sizeof(rg_float));
			return tempbuf != 0;
		}
		else if (val > tempbuf_size)
		{
			tempbuf_size = val;
			rg_float * _tempbuf = (rg_float*)realloc(tempbuf,val * sizeof(rg_float));
			if (_tempbuf==0) return false;
			tempbuf = _tempbuf;
			return true;
		}
		else return true;
	}

	bool AnalyzeSamples ( const rg_float* left_samples, const rg_float* right_samples, unsigned num_samples, unsigned num_channels );
public:
	virtual unsigned GetSampleRate() {return m_srate;}

	bool ProcessSamples ( const rg_input * ptr, unsigned channels, unsigned samples );

	rgcalc_i() : m_srate(0), tempbuf(0), tempbuf_size(0)
	{
	}

	~rgcalc_i()
	{
		if (tempbuf) free(tempbuf);
	}

	rg_float GetTitleGain ( void );

	rg_float GetAlbumGain ( void );

	bool InitGainAnalysis( unsigned samplefreq );

};
//old
static void filter ( const rg_float* input, rg_float* output, size_t nSamples, const rg_float* a, const rg_float* b, size_t order )
{
	double  y;
	size_t  i;
	size_t  k;
	for ( i = 0; i < nSamples; i++ ) {
		y = input[i] * b[0];
		for ( k = 1; k <= order; k++ )
			y += input[i-k] * b[k] - output[i-k] * a[k];
		output[i] = (rg_float)y;
	}
}

static void __fastcall filter10_std ( const rg_float* input, rg_float* output, size_t nSamples, const rg_float* a, const rg_float* b )
{
	double  y;
	size_t  i;
	for ( i = 0; i < nSamples; i++ ) {
		y = input[i] * b[0];

		y += input[i-1] * b[1] - output[i-1] * a[1];
		y += input[i-2] * b[2] - output[i-2] * a[2];
		y += input[i-3] * b[3] - output[i-3] * a[3];
		y += input[i-4] * b[4] - output[i-4] * a[4];
		y += input[i-5] * b[5] - output[i-5] * a[5];
		y += input[i-6] * b[6] - output[i-6] * a[6];
		y += input[i-7] * b[7] - output[i-7] * a[7];
		y += input[i-8] * b[8] - output[i-8] * a[8];
		y += input[i-9] * b[9] - output[i-9] * a[9];
		y += input[i-10] * b[10] - output[i-10] * a[10];



		output[i] = (rg_float)y;
	}
}

typedef void (__fastcall * t_filterfunc) ( const rg_float* input, rg_float* output, size_t nSamples, const rg_float* a, const rg_float* b ) ;
typedef rg_float (__fastcall * t_squared_values)(const rg_float * src,unsigned num);

#ifndef RG_NOASM
extern "C" {
	void _cdecl filter10_3dnow();
	void _cdecl filter2_x87();
	void _cdecl squared_values_x87();
}

static t_filterfunc filter10 = /*detect_sse() ? (t_filterfunc)filter10_sse : */detect_3dnow_ex() ? (t_filterfunc)filter10_3dnow : filter10_std;

#define filter2 ((t_filterfunc)filter2_x87)
#define squared_values ((t_squared_values)squared_values_x87)

#else

#define filter10 filter10_std

static void __fastcall filter2 ( const rg_float* input, rg_float* output, size_t nSamples, const rg_float* a, const rg_float* b)
{
	double  y;
	size_t  i;
	for ( i = 0; i < nSamples; i++ ) {
		y = input[i] * b[0];
		y += input[i-1] * b[1] - output[i-1] * a[1];
		y += input[i-2] * b[2] - output[i-2] * a[2];
		output[i] = (rg_float)y;
	}
}

inline static rg_float squared_values(const rg_float * src,unsigned num)
{
	rg_float acc = 0;
	for(;num;num--)
	{
		acc += (*src) * (*src);
		src++;
	}
	return acc;
}


#endif


static unsigned find_srate_index(unsigned freq)
{
	unsigned n;
	for(n=0;n<N_SRATES;n++)
	{
		if (freq==freqinfos[n].rate) return n;
	}
	return (unsigned)(-1);
}

bool rgcalc::is_supported_samplerate(unsigned freq)
{
	return find_srate_index(freq)!=(unsigned)(-1);
}

// returns a INIT_GAIN_ANALYSIS_OK if successful, INIT_GAIN_ANALYSIS_ERROR if not

bool rgcalc_i::ResetSampleFrequency ( unsigned samplefreq ) {
    int  i;

	if (!is_supported_samplerate(samplefreq)) return false;

	unsigned freqindex = find_srate_index(samplefreq);

	{
		assert(freqindex!=(unsigned)(-1));
		memcpy(l_AYule,freqinfos[freqindex].AYule,tabsize(l_AYule) * sizeof(rg_float));
		memcpy(l_BYule,freqinfos[freqindex].BYule,tabsize(l_BYule) * sizeof(rg_float));
		memcpy(l_AButter,freqinfos[freqindex].AButter,tabsize(l_AButter) * sizeof(rg_float));
		memcpy(l_BButter,freqinfos[freqindex].BButter,tabsize(l_BButter) * sizeof(rg_float));
	}

    // zero out initial values
    for ( i = 0; i < MAX_ORDER; i++ )
        linprebuf[i] = lstepbuf[i] = loutbuf[i] = rinprebuf[i] = rstepbuf[i] = routbuf[i] = 0.;

    sampleWindow = (int) ceil (samplefreq * RMS_WINDOW_TIME);

    lsum         = 0.;
    rsum         = 0.;
    totsamp      = 0;

    memset ( A, 0, sizeof(A) );



#ifndef RG_NOASM
	if (detect_3dnow_ex())
	{
		invert_tab_3dnow(l_AYule,tabsize(l_BYule));
		invert_tab_3dnow(l_BYule,tabsize(l_BYule));
	}
#endif

	return true;
}

bool rgcalc_i::InitGainAnalysis(unsigned samplefreq)
{
	if (!ResetSampleFrequency(samplefreq)) {
		return false;
	}

    linpre       = linprebuf + MAX_ORDER;
    rinpre       = rinprebuf + MAX_ORDER;
    lstep        = lstepbuf  + MAX_ORDER;
    rstep        = rstepbuf  + MAX_ORDER;
    lout         = loutbuf   + MAX_ORDER;
    rout         = routbuf   + MAX_ORDER;

    memset ( B, 0, sizeof(B) );
	m_srate = samplefreq;

    return true;
}


bool rgcalc_i::AnalyzeSamples ( const rg_float* left_samples, const rg_float* right_samples, unsigned num_samples, unsigned num_channels )
{
	rg_profiler(AnalyzeSamples);
    const rg_float*  curleft;
    const rg_float*  curright;
    long            batchsamples;
    long            cursamples;
    long            cursamplepos;

	if (m_srate==0) return false;

    if ( num_samples == 0 ) return true;

    cursamplepos = 0;
    batchsamples = num_samples;

    switch ( num_channels) {
    case  1: right_samples = left_samples; break;
    case  2: break;
    default: return false;
    }

	{
		rg_profiler(copy_1);

		if ( num_samples < MAX_ORDER ) {
			memcpy( linprebuf + MAX_ORDER, left_samples , num_samples * sizeof(rg_float) );
			memcpy( rinprebuf + MAX_ORDER, right_samples, num_samples * sizeof(rg_float) );
		}
		else {
			memcpy( linprebuf + MAX_ORDER, left_samples,  MAX_ORDER * sizeof(rg_float) );
			memcpy( rinprebuf + MAX_ORDER, right_samples, MAX_ORDER * sizeof(rg_float) );
		}
	}

    while ( batchsamples > 0 ) {
        cursamples = (unsigned)batchsamples > sampleWindow-totsamp  ?  sampleWindow - totsamp  :  batchsamples;
        if ( cursamplepos < MAX_ORDER ) {
            curleft  = linpre+cursamplepos;
            curright = rinpre+cursamplepos;
            if (cursamples > MAX_ORDER - cursamplepos )
                cursamples = MAX_ORDER - cursamplepos;
        }
        else {
            curleft  = left_samples  + cursamplepos;
            curright = right_samples + cursamplepos;
        }

		{
			rg_profiler(filters10);
//			filter ( curleft , lstep + totsamp, cursamples, AYule[freqindex], BYule[freqindex], YULE_ORDER );
//			filter ( curright, rstep + totsamp, cursamples, AYule[freqindex], BYule[freqindex], YULE_ORDER );

			filter10 ( curleft , lstep + totsamp, cursamples, l_AYule, l_BYule );
			filter10 ( curright, rstep + totsamp, cursamples, l_AYule, l_BYule );

		}
		{
			rg_profiler(filters2);
//		    filter ( lstep + totsamp, lout + totsamp, cursamples, AButter[freqindex], BButter[freqindex], BUTTER_ORDER );
//			filter ( rstep + totsamp, rout + totsamp, cursamples, AButter[freqindex], BButter[freqindex], BUTTER_ORDER );

		    filter2 ( lstep + totsamp, lout + totsamp, cursamples, l_AButter, l_BButter );
			filter2 ( rstep + totsamp, rout + totsamp, cursamples, l_AButter, l_BButter );

		}

		{
			rg_profiler(_squared_values);

			lsum += squared_values(lout+totsamp,cursamples);
			rsum += squared_values(rout+totsamp,cursamples);

		}

        batchsamples -= cursamples;
        cursamplepos += cursamples;
        totsamp      += cursamples;
        if ( totsamp == sampleWindow ) {  // Get the Root Mean Square (RMS) for this set of samples
			rg_profiler(root_mean_square);
            double  val  = STEPS_per_dB * 10. * log10 ( (lsum+rsum) / totsamp * 0.5 * (double)0x40000000 + 1.e-37 );	//0x40000000 added to counter conversion from -0x7FFF .. 0x8000 scale to -1 .. 1
            int     ival = (int) val;
            if ( ival <                     0 ) ival = 0;
            if ( ival >= sizeof(A)/sizeof(*A) ) ival = sizeof(A)/sizeof(*A) - 1;
            A [ival]++;
            lsum = rsum = 0.;
            memmove( loutbuf , loutbuf  + totsamp, MAX_ORDER * sizeof(rg_float));
            memmove( routbuf , routbuf  + totsamp, MAX_ORDER * sizeof(rg_float));
            memmove( lstepbuf, lstepbuf + totsamp, MAX_ORDER * sizeof(rg_float));
            memmove( rstepbuf, rstepbuf + totsamp, MAX_ORDER * sizeof(rg_float));
            totsamp = 0;
        }
        if ( totsamp > sampleWindow )   // somehow I really screwed up: Error in programming! Contact author about totsamp > sampleWindow
		{
			assert(0);
            return false;
		}
    }
 
	{
		rg_profiler(copy_2);

		if ( num_samples < MAX_ORDER ) {
			memmove( linprebuf, linprebuf + num_samples, (MAX_ORDER-num_samples) * sizeof(rg_float) );
			memmove( rinprebuf, rinprebuf + num_samples, (MAX_ORDER-num_samples) * sizeof(rg_float) );
			memcpy( linprebuf + MAX_ORDER - num_samples, left_samples,  num_samples * sizeof(rg_float) );
			memcpy( rinprebuf + MAX_ORDER - num_samples, right_samples, num_samples * sizeof(rg_float) );
		}
		else {
			memcpy( linprebuf, left_samples  + num_samples - MAX_ORDER, MAX_ORDER * sizeof(rg_float));
			memcpy( rinprebuf, right_samples + num_samples - MAX_ORDER, MAX_ORDER * sizeof(rg_float));
		}
	}

    return true;
}


rg_float rgcalc_i::analyzeResult ( Uint32_t* Array, size_t len )
{
    Uint32_t  elems;
    Int32_t   upper;
    size_t    i;

    elems = 0;
    for ( i = 0; i < len; i++ )
        elems += Array[i];
    if ( elems == 0 )
        return 0;//error

    upper = (Int32_t) ceil (elems * (1. - RMS_PERCENTILE));
    for ( i = len; i-- > 0; ) {
        if ( (upper -= Array[i]) <= 0 )
            break;
    }

    return (rg_float) ((rg_float)PINK_REF - (rg_float)i / (rg_float)STEPS_per_dB);
}

rg_float rgcalc_i::GetTitleGain ( void )
{
    rg_float  retval;
    int    i;

    retval = analyzeResult ( A, sizeof(A)/sizeof(*A) );

    for ( i = 0; i < sizeof(A)/sizeof(*A); i++ ) {
        B[i] += A[i];
        A[i]  = 0;
    }

    for ( i = 0; i < MAX_ORDER; i++ )
        linprebuf[i] = lstepbuf[i] = loutbuf[i] = rinprebuf[i] = rstepbuf[i] = routbuf[i] = 0.f;

    totsamp = 0;
    lsum    = rsum = 0.;
    return retval;
}


rg_float rgcalc_i::GetAlbumGain ( void )
{
    return analyzeResult ( B, sizeof(B)/sizeof(*B) );
}


bool rgcalc_i::ProcessSamples ( const rg_input * data, unsigned nch, unsigned samples )
{
	rg_profiler(ProcessSamples);
	if (nch == 6)
	{
		if (!check_tempbuf_size(samples*2)) return false;
		rg_float * left = tempbuf, *right = tempbuf + samples;
		unsigned n;
		const rg_input * src = data;
		for(n=0;n<samples;n++)
		{//L/R/C/LF/BL/BR
			*(left++)   =  (rg_float)(src[0] + src[2]*SQRTHALF + src[4]*SQRTHALF + src[3]);
			*(right++)  =  (rg_float)(src[1] + src[2]*SQRTHALF + src[5]*SQRTHALF + src[3]);
			src+=6;
		}
		return AnalyzeSamples(tempbuf,tempbuf+samples,samples,2);
	}
	else if (nch>2)
	{
		if (!check_tempbuf_size(samples)) return false;
		unsigned n;
		const rg_input * src = data;
		rg_float * out = tempbuf;
		rg_input div = 1.0 / (rg_input) nch;
		for(n=samples;n;n--)
		{
			unsigned nc;
			rg_input v = 0;
			for(nc=nch;nc;nc--)
				v += *(src++);
			v *= div;
			*(out++) = (rg_float) (v) ;
		}
		return AnalyzeSamples(tempbuf,0,samples,1);
	}
	else if (nch==2)
	{
		if (!check_tempbuf_size(samples*2)) return false;
		const rg_input * src = data;
		rg_float * left = tempbuf, *right = tempbuf + samples;

		unsigned n;
		for(n=samples;n;n--)
		{
			*(left++)  = (rg_float)(*(src++));
			*(right++) = (rg_float)(*(src++));
		}

		return AnalyzeSamples(tempbuf,tempbuf+samples,samples,2);
	}
	else//nch==1
	{
		if (!check_tempbuf_size(samples)) return false;
		const rg_input * src=data;

		rg_float * out = tempbuf;
		unsigned n;
		for(n=samples;n;n--)
			*(out++) = (rg_float)(*(src++));

		return AnalyzeSamples(tempbuf,tempbuf,samples,1);
	}
}

rgcalc * rgcalc::create() {return new rgcalc_i;}