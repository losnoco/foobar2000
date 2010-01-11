#ifndef _mpp_dec_h_
#define _mpp_dec_h_

#define MPCDEC_HAVE_ASM


#ifdef MPCDEC_HAVE_ASM
#define ASM_ALIGN(X) __declspec(align(X))
#else
#define ASM_ALIGN(X)
#endif

#define ASM_ALIGN32 ASM_ALIGN(32)

#ifndef _in_mpc_h_
class StreamInfo;
#endif


#define TO_FLOAT    1
#define TO_PCM      2

#ifndef MPC_DECODE
  #define MPC_DECODE    TO_FLOAT    // decode to 32 bit floats ( -1.000...+1.000 )
//#define MPC_DECODE    TO_PCM      // decode to 16 bit PCM    ( -32768...+32767 )
#endif


#if   MPC_DECODE == TO_FLOAT
  #define MPC_SAMPLE_FORMAT   float
  #define MPC_SAMPLE_BPS      32
  #define MPC_SAMPLE_TYPE     "FLOAT"
#elif MPC_DECODE == TO_PCM
  #define MPC_SAMPLE_FORMAT   short
  #define MPC_SAMPLE_BPS      16
  #define MPC_SAMPLE_TYPE     "PCM"
#else
  #error MPC_DECODE incorrect or undefined.
#endif


#define EQ_TAP          13                      // length of FIR filter for EQ
#define DELAY           ((EQ_TAP + 1) / 2)      // delay of FIR
#define FIR_BANDS       4                       // number of subbands to be FIR filtered

#define MEMSIZE         16384                   // overall buffer size
#define MEMSIZE2        (MEMSIZE/2)             // size of one buffer
#define MEMMASK         (MEMSIZE-1)
#define V_MEM           2304
#define FRAMELEN        (36 * 32)               // samples per frame
#define SYNTH_DELAY     481



class MPC_Reader {
public:
  virtual unsigned read ( void *ptr, unsigned size ) = 0;
  virtual bool seek ( unsigned offset ) = 0;
  virtual unsigned tell () = 0;
  virtual unsigned get_size () = 0;
  virtual bool can_seek() = 0;
};

class MPC_decoder {
public:
  MPC_decoder (  );
  ~MPC_decoder ();

  void SetStreamInfo ( StreamInfo *si );
  int FileInit (MPC_Reader & p_reader);
  void ScaleOutput ( double factor );
  float ProcessReplayGain ( int mode, StreamInfo *info );

  unsigned decode ( MPC_Reader & p_reader, MPC_SAMPLE_FORMAT *buffer, unsigned * vbr_update_acc, unsigned * vbr_update_bits );

  void UpdateBuffer ( MPC_Reader & p_reader, unsigned int RING );
  int seek ( MPC_Reader & p_reader, double seconds );

  void RESET_Synthesis ( void );
  void RESET_Globals ( void );

private:
	unsigned int BitsRead ( void );
	unsigned decode_internal ( MPC_SAMPLE_FORMAT *buffer );
	void RESET_Y ( void );
	void Lese_Bitstrom_SV6 ( void );
	void Lese_Bitstrom_SV7 ( void );

	void Requantisierung ( const int Last_Band );

	unsigned samples_to_skip;

	typedef struct {
	    int  L [36];
	    int  R [36];
	} QuantTyp;

	typedef struct {
	    unsigned int  Code;
		unsigned int  Length;
		int           Value;
	} HuffmanTyp;

	static int HuffmanTyp_cmpfn ( const void* p1, const void* p2 );

private:
  unsigned int  Speicher [MEMSIZE];         // read-buffer
  unsigned int  dword;                      // actually decoded 32bit-word
  unsigned int  pos;                        // bit-position within dword
  unsigned int  Zaehler;                    // actual index within read-buffer

  unsigned int  FwdJumpInfo;
  unsigned int  ActDecodePos;
  unsigned int  FrameWasValid;

  unsigned int  DecodedFrames;
  unsigned int  OverallFrames;
  int           SectionBitrate;             // average bitrate for short section
  int           SampleRate;                 // Sample frequency
  int           NumberOfConsecutiveBrokenFrames;  // counter for consecutive broken frames

private:
  unsigned int  StreamVersion;              // version of bitstream
  unsigned int  MS_used;                    // MS-coding used ?
  int           Max_Band;
  unsigned int  MPCHeaderPos;               // AB: needed to support ID3v2
  //unsigned int  OverallFrames;
  //unsigned int  DecodedFrames;
  unsigned int  LastValidSamples;
  unsigned int  TrueGaplessPresent;

  unsigned int  EQ_activated;

  unsigned int  WordsRead;                  // counts amount of decoded dwords

  /*static*/ unsigned int  __r1;
  /*static*/ unsigned int  __r2;

//  unsigned long clips;

private:

  int           SCF_Index_L [32] [3];
  int           SCF_Index_R [32] [3];       // holds scalefactor-indices
  QuantTyp      Q [32];                     // holds quantized samples
  int           Res_L [32];
  int           Res_R [32];                 // holds the chosen quantizer for each subband
  int           DSCF_Flag_L [32];
  int           DSCF_Flag_R [32];           // differential SCF used?
  int           SCFI_L [32];
  int           SCFI_R [32];                // describes order of transmitted SCF
  int           DSCF_Reference_L [32];
  int           DSCF_Reference_R [32];      // holds last frames SCF
  int           MS_Flag[32];                // MS used?

  float         SAVE_L     [DELAY] [32];    // buffer for ...
  float         SAVE_R     [DELAY] [32];    // ... upper subbands
  float         FirSave_L  [FIR_BANDS] [EQ_TAP];// buffer for ...
  float         FirSave_R  [FIR_BANDS] [EQ_TAP];// ... lowest subbands

  HuffmanTyp    HuffHdr  [10];
  HuffmanTyp    HuffSCFI [ 4];
  HuffmanTyp    HuffDSCF [16];
  HuffmanTyp*   HuffQ [2] [8];

  HuffmanTyp    HuffQ1 [2] [3*3*3];
  HuffmanTyp    HuffQ2 [2] [5*5];
  HuffmanTyp    HuffQ3 [2] [ 7];
  HuffmanTyp    HuffQ4 [2] [ 9];
  HuffmanTyp    HuffQ5 [2] [15];
  HuffmanTyp    HuffQ6 [2] [31];
  HuffmanTyp    HuffQ7 [2] [63];

  const HuffmanTyp* SampleHuff [18];
  HuffmanTyp    SCFI_Bundle   [ 8];
  HuffmanTyp    DSCF_Entropie [13];
  HuffmanTyp    Region_A [16];
  HuffmanTyp    Region_B [ 8];
  HuffmanTyp    Region_C [ 4];

  HuffmanTyp    Entropie_1 [ 3];
  HuffmanTyp    Entropie_2 [ 5];
  HuffmanTyp    Entropie_3 [ 7];
  HuffmanTyp    Entropie_4 [ 9];
  HuffmanTyp    Entropie_5 [15];
  HuffmanTyp    Entropie_6 [31];
  HuffmanTyp    Entropie_7 [63];

  float         V_L [V_MEM + 960];
  float         V_R [V_MEM + 960];
  float         Y_L [36] [32];
  float         Y_R [36] [32];

  float         SCF  [256];                 // holds adapted scalefactors (for clipping prevention)
  unsigned int  Q_bit [32];                 // holds amount of bits to save chosen quantizer (SV6)
  unsigned int  Q_res [32] [16];            // holds conversion: index -> quantizer (SV6)

private: // functions
  void Reset_BitstreamDecode ( void );
  unsigned int Bitstream_read ( const unsigned int );
  int Huffman_Decode ( const HuffmanTyp* );         // works with maximum lengths up to 14
  int Huffman_Decode_fast ( const HuffmanTyp* );    // works with maximum lengths up to 10
  int Huffman_Decode_faster ( const HuffmanTyp* );  // works with maximum lengths up to  5
  void SCFI_Bundle_read ( const HuffmanTyp*, int*, int* );

  //void GenerateDither_old ( float* p, size_t len );
  //void GenerateDither ( float* p, size_t len );

  void Reset_V ( void );
  void Synthese_Filter_float ( float * dst );
  unsigned int random_int ( void );

  void EQSet ( int on, char data[10], int preamp );

  void Helper1 ( MPC_Reader & p_reader, unsigned long bitpos );
  void Helper2 ( MPC_Reader & p_reader, unsigned long bitpos );
  void Helper3 ( MPC_Reader & p_reader, unsigned long bitpos, unsigned long* buffoffs );

  void Huffman_SV6_Encoder ( void );
  void Huffman_SV6_Decoder ( void );
  void Huffman_SV7_Encoder ( void );
  void Huffman_SV7_Decoder ( void );

  void initialisiere_Quantisierungstabellen ( void );
  void Quantisierungsmodes ( void );        // conversion: index -> quantizer (bitstream reading)
  void Resort_HuffTables ( const unsigned int elements, HuffmanTyp* Table, const int offset );

};

#endif
