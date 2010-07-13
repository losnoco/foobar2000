/////////////////////////////////////////////////////////////////////////////
//
// spucore - Emulates a single SPU CORE
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "spucore.h"

////////////////////////////////////////////////////////////////////////////////
/*
** Key-on defer
**
** Bit of a hack I did to stop the clicking in Final Fantasy Tactics
** If there's a key-on while the envelope is still active, the key-on is
** deferred by this many samples while the existing channel is microramped
** down to zero (works best if it's a power of 2)
*/

//#define KEYON_DEFER_SAMPLES (64)
#define KEYON_DEFER_SAMPLES (64)

//
// Render max samples 
//
#define RENDERMAX (200)

////////////////////////////////////////////////////////////////////////////////
/*
** Static information
*/
static const sint32 gauss_shuffled_reverse_table[1024] = {
 366,1305, 374,   0, 362,1304, 378,   0, 358,1304, 381,   0, 354,1304, 385,   0, 351,1304, 389,   0, 347,1304, 393,   0, 343,1303, 397,   0, 339,1303, 401,   0,
 336,1303, 405,   0, 332,1302, 410,   0, 328,1302, 414,   0, 325,1301, 418,   0, 321,1300, 422,   0, 318,1300, 426,   0, 314,1299, 430,   0, 311,1298, 434,   0,
 307,1297, 439,   1, 304,1297, 443,   1, 300,1296, 447,   1, 297,1295, 451,   1, 293,1294, 456,   1, 290,1293, 460,   1, 286,1292, 464,   1, 283,1291, 469,   1,
 280,1290, 473,   1, 276,1288, 477,   1, 273,1287, 482,   1, 270,1286, 486,   2, 267,1284, 491,   2, 263,1283, 495,   2, 260,1282, 499,   2, 257,1280, 504,   2,
 254,1279, 508,   2, 251,1277, 513,   2, 248,1275, 517,   3, 245,1274, 522,   3, 242,1272, 527,   3, 239,1270, 531,   3, 236,1269, 536,   3, 233,1267, 540,   4,
 230,1265, 545,   4, 227,1263, 550,   4, 224,1261, 554,   4, 221,1259, 559,   4, 218,1257, 563,   5, 215,1255, 568,   5, 212,1253, 573,   5, 210,1251, 577,   5,
 207,1248, 582,   6, 204,1246, 587,   6, 201,1244, 592,   6, 199,1241, 596,   6, 196,1239, 601,   7, 193,1237, 606,   7, 191,1234, 611,   7, 188,1232, 615,   8,
 186,1229, 620,   8, 183,1227, 625,   8, 180,1224, 630,   9, 178,1221, 635,   9, 175,1219, 640,   9, 173,1216, 644,  10, 171,1213, 649,  10, 168,1210, 654,  10,
 166,1207, 659,  11, 163,1205, 664,  11, 161,1202, 669,  11, 159,1199, 674,  12, 156,1196, 678,  12, 154,1193, 683,  13, 152,1190, 688,  13, 150,1186, 693,  14,
 147,1183, 698,  14, 145,1180, 703,  15, 143,1177, 708,  15, 141,1174, 713,  15, 139,1170, 718,  16, 137,1167, 723,  16, 134,1164, 728,  17, 132,1160, 732,  17,
 130,1157, 737,  18, 128,1153, 742,  19, 126,1150, 747,  19, 124,1146, 752,  20, 122,1143, 757,  20, 120,1139, 762,  21, 118,1136, 767,  21, 117,1132, 772,  22,
 115,1128, 777,  23, 113,1125, 782,  23, 111,1121, 787,  24, 109,1117, 792,  24, 107,1113, 797,  25, 106,1109, 802,  26, 104,1106, 806,  27, 102,1102, 811,  27,
 100,1098, 816,  28,  99,1094, 821,  29,  97,1090, 826,  29,  95,1086, 831,  30,  94,1082, 836,  31,  92,1078, 841,  32,  90,1074, 846,  32,  89,1070, 851,  33,
  87,1066, 855,  34,  86,1061, 860,  35,  84,1057, 865,  36,  83,1053, 870,  36,  81,1049, 875,  37,  80,1045, 880,  38,  78,1040, 884,  39,  77,1036, 889,  40,
  76,1032, 894,  41,  74,1027, 899,  42,  73,1023, 904,  43,  71,1019, 908,  44,  70,1014, 913,  45,  69,1010, 918,  46,  67,1005, 923,  47,  66,1001, 927,  48,
  65, 997, 932,  49,  64, 992, 937,  50,  62, 988, 941,  51,  61, 983, 946,  52,  60, 978, 951,  53,  59, 974, 955,  54,  58, 969, 960,  55,  56, 965, 965,  56,
  55, 960, 969,  58,  54, 955, 974,  59,  53, 951, 978,  60,  52, 946, 983,  61,  51, 941, 988,  62,  50, 937, 992,  64,  49, 932, 997,  65,  48, 927,1001,  66,
  47, 923,1005,  67,  46, 918,1010,  69,  45, 913,1014,  70,  44, 908,1019,  71,  43, 904,1023,  73,  42, 899,1027,  74,  41, 894,1032,  76,  40, 889,1036,  77,
  39, 884,1040,  78,  38, 880,1045,  80,  37, 875,1049,  81,  36, 870,1053,  83,  36, 865,1057,  84,  35, 860,1061,  86,  34, 855,1066,  87,  33, 851,1070,  89,
  32, 846,1074,  90,  32, 841,1078,  92,  31, 836,1082,  94,  30, 831,1086,  95,  29, 826,1090,  97,  29, 821,1094,  99,  28, 816,1098, 100,  27, 811,1102, 102,
  27, 806,1106, 104,  26, 802,1109, 106,  25, 797,1113, 107,  24, 792,1117, 109,  24, 787,1121, 111,  23, 782,1125, 113,  23, 777,1128, 115,  22, 772,1132, 117,
  21, 767,1136, 118,  21, 762,1139, 120,  20, 757,1143, 122,  20, 752,1146, 124,  19, 747,1150, 126,  19, 742,1153, 128,  18, 737,1157, 130,  17, 732,1160, 132,
  17, 728,1164, 134,  16, 723,1167, 137,  16, 718,1170, 139,  15, 713,1174, 141,  15, 708,1177, 143,  15, 703,1180, 145,  14, 698,1183, 147,  14, 693,1186, 150,
  13, 688,1190, 152,  13, 683,1193, 154,  12, 678,1196, 156,  12, 674,1199, 159,  11, 669,1202, 161,  11, 664,1205, 163,  11, 659,1207, 166,  10, 654,1210, 168,
  10, 649,1213, 171,  10, 644,1216, 173,   9, 640,1219, 175,   9, 635,1221, 178,   9, 630,1224, 180,   8, 625,1227, 183,   8, 620,1229, 186,   8, 615,1232, 188,
   7, 611,1234, 191,   7, 606,1237, 193,   7, 601,1239, 196,   6, 596,1241, 199,   6, 592,1244, 201,   6, 587,1246, 204,   6, 582,1248, 207,   5, 577,1251, 210,
   5, 573,1253, 212,   5, 568,1255, 215,   5, 563,1257, 218,   4, 559,1259, 221,   4, 554,1261, 224,   4, 550,1263, 227,   4, 545,1265, 230,   4, 540,1267, 233,
   3, 536,1269, 236,   3, 531,1270, 239,   3, 527,1272, 242,   3, 522,1274, 245,   3, 517,1275, 248,   2, 513,1277, 251,   2, 508,1279, 254,   2, 504,1280, 257,
   2, 499,1282, 260,   2, 495,1283, 263,   2, 491,1284, 267,   2, 486,1286, 270,   1, 482,1287, 273,   1, 477,1288, 276,   1, 473,1290, 280,   1, 469,1291, 283,
   1, 464,1292, 286,   1, 460,1293, 290,   1, 456,1294, 293,   1, 451,1295, 297,   1, 447,1296, 300,   1, 443,1297, 304,   1, 439,1297, 307,   0, 434,1298, 311,
   0, 430,1299, 314,   0, 426,1300, 318,   0, 422,1300, 321,   0, 418,1301, 325,   0, 414,1302, 328,   0, 410,1302, 332,   0, 405,1303, 336,   0, 401,1303, 339,
   0, 397,1303, 343,   0, 393,1304, 347,   0, 389,1304, 351,   0, 385,1304, 354,   0, 381,1304, 358,   0, 378,1304, 362,   0, 374,1305, 366,   0, 370,1305, 370,
};

static sint32 ratelogtable[32+128];

// v1.10 coefs - normalized from v1.04
static const sint32 reverb_lowpass_coefs[8] = {
  (int)((-0.036346113709214548)*(2048.0)),
  (int)(( 0.044484956332843419)*(2048.0)),
  (int)(( 0.183815456609675380)*(2048.0)),
  (int)(( 0.308045700766695740)*(2048.0)),
  (int)(( 0.308045700766695740)*(2048.0)),
  (int)(( 0.183815456609675380)*(2048.0)),
  (int)(( 0.044484956332843419)*(2048.0)),
  (int)((-0.036346113709214548)*(2048.0))
};

/*
// test coefs - ganked from LAME's blackman function
// (11-point filter, but only a few points were nonzero)
static const sint32 reverb_new_lowpass_coefs[3] = {
  (int)((-0.02134438446523164)*(2048.0)),
// skip one
  (int)(( 0.27085135668587779)*(2048.0)),
  (int)(( 0.50098605555870768)*(2048.0))
// rest are symmetrical
};
*/

static const sint32 bit_reverse_table[16] = {
  0, 8, 4, 12, 2, 10, 6, 14,
  1, 9, 5, 13, 3, 11, 7, 15
};

/*
** Static init
*/
sint32 EMU_CALL spucore_init(void) {
  sint32 i;

  memset(ratelogtable, 0, sizeof(ratelogtable));
  ratelogtable[32-8] = 1;
  ratelogtable[32-7] = 1;
  ratelogtable[32-6] = 1;
  ratelogtable[32-5] = 1;
  ratelogtable[32-4] = 2;
  ratelogtable[32-3] = 2;
  ratelogtable[32-2] = 3;
  ratelogtable[32-1] = 3;
  ratelogtable[32+0] = 4;
  ratelogtable[32+1] = 5;
  ratelogtable[32+2] = 6;
  ratelogtable[32+3] = 7;
  for(i = 4; i < 128; i++) {
    uint32 n = 2*ratelogtable[32+i-4];
    if(n > 0x20000000) n = 0x20000000;
    ratelogtable[32+i] = n;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/*
** State information
*/
#define SPUCORESTATE ((struct SPUCORE_STATE*)(state))

#define SAMPLE_STATE_OFF    (0)
#define SAMPLE_STATE_ENDING (1)
#define SAMPLE_STATE_ON     (2)

struct SPUCORE_SAMPLE {
  uint8 state;
  uint8 array_cleared;
  sint32 array[32];
  uint32 phase;

  uint32 block_addr;
  uint32 start_block_addr;
  //uint32 start_loop_block_addr;
  uint32 loop_block_addr;
};

#define ENVELOPE_STATE_OFF     (0)
#define ENVELOPE_STATE_ATTACK  (1)
#define ENVELOPE_STATE_DECAY   (2)
#define ENVELOPE_STATE_SUSTAIN (3)
#define ENVELOPE_STATE_RELEASE (4)

struct SPUCORE_ENVELOPE {
  uint32 reg_ad;
  uint32 reg_sr;
  sint32 level;
  sint32 delta;
  int    state;
  sint32 cachemax;
};

////////////////////////////////////////////////////////////////////////////////
/*
** Volumes with increase/decrease modes
*/
struct SPUCORE_VOLUME {
  uint16 mode;
  sint32 level;
};

static EMU_INLINE void EMU_CALL volume_setmode(struct SPUCORE_VOLUME *vol, uint16 mode) {
  vol->mode = mode;
  if(mode & 0x8000) {
  } else {
    vol->level = mode;
    vol->level <<= 17;
    vol->level >>= 1;
  }
}

static EMU_INLINE uint16 EMU_CALL volume_getmode(struct SPUCORE_VOLUME *vol) {
  return vol->mode;
}

static EMU_INLINE sint32 EMU_CALL volume_getlevel(struct SPUCORE_VOLUME *vol) {
  return (vol->level) >> 15;
}

////////////////////////////////////////////////////////////////////////////////

struct SPUCORE_CHAN {
  struct SPUCORE_VOLUME vol[2];
  uint32 voice_pitch;
  struct SPUCORE_SAMPLE   sample;
  struct SPUCORE_ENVELOPE env;
  int samples_until_pending_keyon;
};

/*
** Reverb resample state
*/
struct SPUCORE_REVERB_RESAMPLER {
  sint32  in_queue_l[8];
  sint32  in_queue_r[8];
  sint32 out_queue_l[16];
  sint32 out_queue_r[16];
  int queue_index;
};

struct SPUCORE_REVERB {
  uint32 FB_SRC_A   ;
  uint32 FB_SRC_B   ;
  uint16 IIR_ALPHA  ;
  uint16 ACC_COEF_A ;
  uint16 ACC_COEF_B ;
  uint16 ACC_COEF_C ;
  uint16 ACC_COEF_D ;
  uint16 IIR_COEF   ;
  uint16 FB_ALPHA   ;
  uint16 FB_X       ;
  uint32 IIR_DEST_A0;
  uint32 IIR_DEST_A1;
  uint32 ACC_SRC_A0 ;
  uint32 ACC_SRC_A1 ;
  uint32 ACC_SRC_B0 ;
  uint32 ACC_SRC_B1 ;
  uint32 IIR_SRC_A0 ;
  uint32 IIR_SRC_A1 ;
  uint32 IIR_DEST_B0;
  uint32 IIR_DEST_B1;
  uint32 ACC_SRC_C0 ;
  uint32 ACC_SRC_C1 ;
  uint32 ACC_SRC_D0 ;
  uint32 ACC_SRC_D1 ;
  uint32 IIR_SRC_B1 ;
  uint32 IIR_SRC_B0 ;
  uint32 MIX_DEST_A0;
  uint32 MIX_DEST_A1;
  uint32 MIX_DEST_B0;
  uint32 MIX_DEST_B1;
  uint16 IN_COEF_L  ;
  uint16 IN_COEF_R  ;

  uint32 start_address;
  uint32 end_address;

  sint32 current_address;

  sint32 safe_start_address;
  sint32 safe_end_address;
  sint32 safe_size;

  struct SPUCORE_REVERB_RESAMPLER resampler;
};

struct SPUCORE_STATE {
  uint32 flags;
  sint32 memsize;
  struct SPUCORE_CHAN chan[24];
  struct SPUCORE_REVERB reverb;
  struct SPUCORE_VOLUME mvol[2];
  sint16 evol[2];
  sint16 avol[2];
  sint16 bvol[2];
  uint32 kon;
  uint32 koff;
  uint32 fm;
  uint32 noise;
  uint32 vmix[2];
  uint32 vmixe[2];
  uint32 irq_address;
  uint32 noiseclock;
  uint32 noisecounter;
  sint32 noiseval;
  uint32 irq_decoder_clock;
  uint32 irq_triggered_cycle;
};

struct SPUCORE_IRQ_STATE {
  uint32 offset;
  uint32 triggered_cycle;
};

uint32 EMU_CALL spucore_get_state_size(void) {
  return(sizeof(struct SPUCORE_STATE));
}

/*
** Initialize SPU CORE state
*/
void EMU_CALL spucore_clear_state(void *state) {
  /*
  ** Clear to zero
  */
  memset(state, 0, sizeof(struct SPUCORE_STATE));
  /*
  ** Set other defaults
  */
  SPUCORESTATE->memsize = 0x80000;
  spucore_setreg(state, SPUREG_EEA, 0x7FFFF, 0xFFFFFFFF);
  spucore_setreg(state, SPUREG_VMIX, 0x00FFFFFF, 0xFFFFFFFF);
  spucore_setflag(state, SPUREG_FLAG_MSNDL , 1);
  spucore_setflag(state, SPUREG_FLAG_MSNDR , 1);
  spucore_setflag(state, SPUREG_FLAG_MSNDEL, 1);
  spucore_setflag(state, SPUREG_FLAG_MSNDER, 1);
  spucore_setflag(state, SPUREG_FLAG_SINL, 1);
  spucore_setflag(state, SPUREG_FLAG_SINR, 1);
  SPUCORESTATE->noiseval = 1;
  SPUCORESTATE->irq_triggered_cycle = 0xFFFFFFFF;
}

void EMU_CALL spucore_set_mem_size(void *state, uint32 size) {
  SPUCORESTATE->memsize = (sint32)size;
  spucore_setreg(state, SPUREG_EEA, size-1, 0xFFFFFFFF);
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL make_safe_reverb_addresses(struct SPUCORE_STATE *state) {
  struct SPUCORE_REVERB *r = &(state->reverb);
  sint32 sa = r->start_address;
  sint32 ea = r->end_address;

//EMUTRACE2("[sa,ea=%X,%X]\n",sa,ea);

  ea += 0x20000;
  ea &= (~0x1FFFF);
  sa &= (~1);

  if(ea > state->memsize) ea = state->memsize;
  if(ea < 0x20000) ea = 0x20000;
  if(sa > ea) {
    sa &= 0x1FFFE;
    sa += ea;
    sa -= 0x20000;
  }

  r->safe_start_address = sa;
  r->safe_end_address   = ea;
  r->safe_size          = ea-sa;

  r->current_address &= (~1);
  if(
    (r->current_address < r->safe_start_address) ||
    (r->current_address >= r->safe_end_address)
  ) {
    r->current_address = r->safe_start_address;
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** ADPCM sample decoding
*/
/*
#define SPUCORE_PREDICT_SKEL(prednum,coef1,coef2) \
static void EMU_CALL spucore_predict_##prednum(uint8 *src, sint32 *dest, uint32 shift) {  \
  uint32 i;                                                                               \
  sint32 p_a = dest[-2];                                                                  \
  sint32 p_b = dest[-1];                                                                  \
  shift += 16;                                                                            \
  for(i = 0; i < 14; i++) {                                                               \
    sint32 a = src[i^(EMU_ENDIAN_XOR(1))];                                                \
    sint32 b = (a&0xF0)<<24;                                                              \
    a = a << 28; b >>= shift; a >>= shift;                                                \
    a += ( ( ((coef1)*p_b) + ((coef2)*p_a) )+32)>>6;                                      \
    if(a > 32767) { a = 32767; } if(a < (-32768)) { a = (-32768); }                       \
    *dest++ = a;                                                                          \
    b += ( ( ((coef1)* a ) + ((coef2)*p_b) )+32)>>6;                                      \
    if(b > 32767) { b = 32767; } if(b < (-32768)) { b = (-32768); }                       \
    *dest++ = b;                                                                          \
    p_a = a; p_b = b;                                                                     \
  }                                                                                       \
}
*/

#define SPUCORE_PREDICT_SKEL(prednum,coef1,coef2) \
static void EMU_CALL spucore_predict_##prednum(uint16 *src, sint32 *dest, uint32 shift) { \
  uint32 i;                                                                               \
  sint32 p_a = dest[-2];                                                                  \
  sint32 p_b = dest[-1];                                                                  \
  shift += 16;                                                                            \
  for(i = 0; i < 7; i++) {                                                                \
    sint32 a = *src++;                                                                    \
    sint32 b = (a&0x00F0)<<24;                                                            \
    sint32 c = (a&0x0F00)<<20;                                                            \
    sint32 d = (a&0xF000)<<16;                                                            \
    a <<= 28;                                                                             \
    a >>= shift;                                                                          \
    b >>= shift;                                                                          \
    c >>= shift;                                                                          \
    d >>= shift;                                                                          \
    a += ( ( ((coef1)*p_b) + ((coef2)*p_a) )+32)>>6;                                      \
    if(a > 32767) { a = 32767; } if(a < (-32768)) { a = (-32768); }                       \
    *dest++ = a;                                                                          \
    b += ( ( ((coef1)* a ) + ((coef2)*p_b) )+32)>>6;                                      \
    if(b > 32767) { b = 32767; } if(b < (-32768)) { b = (-32768); }                       \
    *dest++ = b;                                                                          \
    c += ( ( ((coef1)* b ) + ((coef2)* a ) )+32)>>6;                                      \
    if(b > 32767) { b = 32767; } if(b < (-32768)) { b = (-32768); }                       \
    *dest++ = c;                                                                          \
    d += ( ( ((coef1)* c ) + ((coef2)* b ) )+32)>>6;                                      \
    if(b > 32767) { b = 32767; } if(b < (-32768)) { b = (-32768); }                       \
    *dest++ = d;                                                                          \
    p_a = c; p_b = d;                                                                     \
  }                                                                                       \
}

SPUCORE_PREDICT_SKEL(0,0,0)
SPUCORE_PREDICT_SKEL(1,60,0)
SPUCORE_PREDICT_SKEL(2,115,-52)
SPUCORE_PREDICT_SKEL(3,98,-55)
SPUCORE_PREDICT_SKEL(4,122,-60)

typedef void (EMU_CALL * spucore_predict_callback_t)(uint16*, sint32*, uint32);

static spucore_predict_callback_t spucore_predict[8] = {
  spucore_predict_0, spucore_predict_1, spucore_predict_2, spucore_predict_3,
  spucore_predict_4, spucore_predict_1, spucore_predict_2, spucore_predict_3
};

static void EMU_CALL decode_sample_block(
  uint16 *ram,
  uint32 memmax,
  struct SPUCORE_SAMPLE *sample,
  int skip
) {
//  uint32 i;
  if(sample->state != SAMPLE_STATE_ON) {
    if(!sample->array_cleared) {
      memset(sample->array, 0, sizeof(sample->array));
      sample->array_cleared = 1;
    }
    sample->state = SAMPLE_STATE_OFF;
  } else {
    /* temporary hack to avoid memory problems */
    sample->block_addr &= (memmax-1);
    if((sample->block_addr + 0x10) > memmax) sample->block_addr -= 0x10;
    ram += sample->block_addr >> 1;
    /* decode */
    if(skip) {
      if(!sample->array_cleared) {
        memset(sample->array, 0, sizeof(sample->array));
        sample->array_cleared = 1;
      }
    } else {
      sample->array[0] = sample->array[28];
      sample->array[1] = sample->array[29];
      sample->array[2] = sample->array[30];
      sample->array[3] = sample->array[31];
      (spucore_predict[(ram[0]>>4)&7])(
        ram + 1,
        sample->array + 4,
        ram[0] & 0xF
      );
    }
    /* set loop start address if necessary */
    if(ram[0]&0x0400) {
      sample->loop_block_addr = sample->block_addr;
    }
    /* sample end? */
    if(ram[0]&0x0100) {
      /* loop? */
      if(ram[0]&0x0200) {
        sample->block_addr = sample->loop_block_addr;
      /* no loop, just end */
      } else {
        sample->state = SAMPLE_STATE_ENDING;
      }
    } else {
      /* advance to next block */
      sample->block_addr += 16;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns the number of samples actually generated
// If dest is null, it means "fast forward" - don't render anything
//
//
// output of resampler() is guaranteed clipped, as long as the output of
// decode_sample_block() is
//
static uint32 EMU_CALL resampler(
  uint16 *ram,
  uint32 memmax,
  struct SPUCORE_SAMPLE *sample,
  sint32 *dest,
  uint32 n,
  uint32 phase_inc,
  struct SPUCORE_IRQ_STATE *irq_state
) {
  uint32 irq_triggered_cycle = 0xFFFFFFFF;
  uint32 s;
  uint32 ph; //, phl;
  ph = sample->phase;
  if(!dest) {
    ph += phase_inc * n;
    s = 0;
    while(ph >= 0x1C000) {
      if(sample->state == SAMPLE_STATE_OFF) break;
      if(irq_state && irq_state->offset - sample->block_addr < 16 && irq_triggered_cycle == 0xFFFFFFFF) {
        irq_triggered_cycle = s;
      }
      decode_sample_block(ram, memmax, sample, 1);
      s += phase_inc * 28;
      ph -= 0x1C000;
    }
    s = n;
  } else {
    uint32 t = 0;
    for(s = 0; s < n; s++) {
      sint32 *source_signal;
      sint32 *mygauss;
      sint32 sum;
      if(ph >= 0x1C000) {
        if(sample->state == SAMPLE_STATE_OFF) break;
        if(irq_state && irq_state->offset - sample->block_addr < 16 && irq_triggered_cycle == 0xFFFFFFFF) {
          irq_triggered_cycle = t;
        }
        decode_sample_block(ram, memmax, sample, 0);
        ph -= 0x1C000;
      }
      source_signal = sample->array + (ph >> 12);
      mygauss = (sint32*) (((uint8*)gauss_shuffled_reverse_table) + (ph & 0xFF0));

      { sum =
          (source_signal[0] * mygauss[0]) +
          (source_signal[1] * mygauss[1]) +
          (source_signal[2] * mygauss[2]) +
          (source_signal[3] * mygauss[3]);
      }
      sum >>= 11;

      *dest++ = sum;
      ph += phase_inc;
      t += phase_inc;
    }
  }

  if(irq_state && irq_triggered_cycle != 0xFFFFFFFF) {
    irq_triggered_cycle = (irq_triggered_cycle * 768) >> 12;
    if(irq_triggered_cycle < irq_state->triggered_cycle) irq_state->triggered_cycle = irq_triggered_cycle;
  }

  sample->phase = ph;

  return s;
}

////////////////////////////////////////////////////////////////////////////////

static uint32 EMU_CALL resampler_modulated(
  uint16 *ram,
  uint32 memmax,
  struct SPUCORE_SAMPLE *sample,
  sint32 *dest,
  uint32 n,
  uint32 phase_inc,
  sint32 *fmbuf,
  struct SPUCORE_IRQ_STATE *irq_state
) {
  uint32 irq_triggered_cycle = 0xFFFFFFFF;
  uint32 s, t = 0;
  uint32 ph; //, phl;
  uint32 pimod;
  ph = sample->phase;
  if(!dest) {
    for(s = 0; s < n; s++) {
      pimod = ((*fmbuf++ + 32768) * phase_inc) / 32768;
      if(pimod > 0x3FFF) pimod = 0x3FFF;
      else if(pimod < 1) pimod = 1;
      ph += pimod;
      while(ph >= 0x1C000) {
        if(sample->state == SAMPLE_STATE_OFF) break;
        if(irq_state && irq_state->offset - sample->block_addr < 16 && irq_triggered_cycle == 0xFFFFFFFF) {
          irq_triggered_cycle = t;
        }
        decode_sample_block(ram, memmax, sample, 1);
        ph -= 0x1C000;
      }
      t += pimod;
    }
  } else {
    for(s = 0; s < n; s++) {
      sint32 *source_signal;
      sint32 *mygauss;
      sint32 sum;
      if(ph >= 0x1C000) {
        if(sample->state == SAMPLE_STATE_OFF) break;
        if(irq_state && irq_state->offset - sample->block_addr < 16 && irq_triggered_cycle == 0xFFFFFFFF) {
          irq_triggered_cycle = t;
        }
        decode_sample_block(ram, memmax, sample, 0);
        ph -= 0x1C000;
      }
      source_signal = sample->array + (ph >> 12);
      mygauss = (sint32*) (((uint8*)gauss_shuffled_reverse_table) + (ph & 0xFF0));

      { sum =
          (source_signal[0] * mygauss[0]) +
          (source_signal[1] * mygauss[1]) +
          (source_signal[2] * mygauss[2]) +
          (source_signal[3] * mygauss[3]);
      }
      sum >>= 11;

      *dest++ = sum;
      pimod = ((*fmbuf++ + 32768) * phase_inc) / 32768;
      if(pimod > 0x3FFF) pimod = 0x3FFF;
      else if(pimod < 1) pimod = 1;
      ph += pimod;
      t += pimod;
    }
  }

  if(irq_state && irq_triggered_cycle != 0xFFFFFFFF) {
    irq_triggered_cycle = (irq_triggered_cycle * 768) >> 12;
    if(irq_triggered_cycle < irq_state->triggered_cycle) irq_state->triggered_cycle = irq_triggered_cycle;
  }

  sample->phase = ph;

  return s;
}

////////////////////////////////////////////////////////////////////////////////

#define MY_AM (((env->reg_ad)>>15)&0x01)
#define MY_AR (((env->reg_ad)>> 8)&0x7F)
#define MY_DR (((env->reg_ad)>> 4)&0x0F)
#define MY_SL (((env->reg_ad)>> 0)&0x0F)
#define MY_SM (((env->reg_sr)>>15)&0x01)
#define MY_SD (((env->reg_sr)>>14)&0x01)
#define MY_SR (((env->reg_sr)>> 6)&0x7F)
#define MY_RM (((env->reg_sr)>> 5)&0x01)
#define MY_RR (((env->reg_sr)>> 0)&0x1F)

/*
** - Sets the current envelope slope
** - Returns the max number of samples that can be processed at the current
**   slope
*/
static EMU_INLINE sint32 EMU_CALL envelope_do(struct SPUCORE_ENVELOPE *env) {
  sint32 target = 0;
  /*
  ** Clip envelope value in case it wrapped around
  */
  switch(((env->level) >> 30) & 3) {
  case 2: env->level = 0x7FFFFFFF; break;
  case 3: env->level = 0x00000000; break;
  }
  switch(env->state) {
  case ENVELOPE_STATE_ATTACK : goto attack;
  case ENVELOPE_STATE_DECAY  : goto decay;
  case ENVELOPE_STATE_SUSTAIN: goto sustain;
  case ENVELOPE_STATE_RELEASE: goto release;
  default:
    env->state = ENVELOPE_STATE_OFF;
    env->level = 0;
    env->delta = 0;
    return 1;
  }
attack:
  if(env->level == 0x7FFFFFFF) {
    env->state = ENVELOPE_STATE_DECAY;
    goto decay;
  }
  /* log */
  if(MY_AM) {
    if(env->level < 0x60000000) {
      target = 0x60000000;
      env->delta = ratelogtable[32+(MY_AR^0x7F)-0x10];
    } else {
      target = 0x7FFFFFFF;
      env->delta = ratelogtable[32+(MY_AR^0x7F)-0x18];
    }
  /* linear */
  } else {
    target = 0x7FFFFFFF;
    env->delta = ratelogtable[32+(MY_AR^0x7F)-0x10];
  }
  goto domax;

decay:
  if((((env->level)>>27)&0xF) <= ((sint32)(MY_SL))) {
    env->state = ENVELOPE_STATE_SUSTAIN;
    goto sustain;
  }
  target = env->level & (~0x07FFFFFF);
  switch(((env->level)>>28)&0x7) {
  case 0: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+0];  break;
  case 1: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+4];  break;
  case 2: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+6];  break;
  case 3: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+8];  break;
  case 4: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+9];  break;
  case 5: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+10]; break;
  case 6: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+11]; break;
  case 7: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+12]; break;
  }
  goto domax;

sustain:
  if(!MY_SD) {
    if(env->level == 0x7FFFFFFF) {
      env->delta = 0;
      return 0x7FFFFFFF;
    }
    /* log */
    if(MY_SM) {
      if(env->level < 0x60000000) {
        target = 0x60000000;
        env->delta = ratelogtable[32+(MY_SR^0x7F)-0x10];
      } else {
        target = 0x7FFFFFFF;
        env->delta = ratelogtable[32+(MY_SR^0x7F)-0x18];
      }
    /* linear */
    } else {
      target = 0x7FFFFFFF;
      env->delta = ratelogtable[32+(MY_SR^0x7F)-0x10];
    }
  } else {
    if(env->level == 0x00000000) {
      env->delta = 0;
      return 0x7FFFFFFF;
    }
    /* log */
    if(MY_SM) {
      target = ((env->level)&(~0x0FFFFFFF));
      switch(((env->level)>>28)&0x7) {
      case 0: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+0];  break;
      case 1: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+4];  break;
      case 2: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+6];  break;
      case 3: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+8];  break;
      case 4: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+9];  break;
      case 5: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+10]; break;
      case 6: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+11]; break;
      case 7: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+12]; break;
      }
    /* linear */
    } else {
      target = 0x00000000;
      env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x0F];
    }
  }
  goto domax;

release:
  if(env->level == 0) {
    env->delta = 0;
    env->state = ENVELOPE_STATE_OFF;
    return 1;
  }
  /* log */
  if(MY_RM) {
    target = ((env->level) & (~0x0FFFFFFF));
    switch(((env->level)>>28)&0x7) {
    case 0: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+0];  break;
    case 1: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+4];  break;
    case 2: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+6];  break;
    case 3: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+8];  break;
    case 4: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+9];  break;
    case 5: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+10]; break;
    case 6: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+11]; break;
    case 7: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+12]; break;
    }
  /* linear */
  } else {
    target = 0;
    env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x0C];
  }
  goto domax;

domax:
  { sint32 max;
    if(env->delta) {
      max = (target - (env->level)) / (env->delta);
    } else {
      max = 0x7FFFFFFF;
    }
    max--;
    if(max < 1) max = 1;
    return max;
  }
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL envelope_prime(struct SPUCORE_ENVELOPE *env) {
//  if(env->state != ENVELOPE_STATE_OFF) {
//if(env->state!=ENVELOPE_STATE_RELEASE){
//    OutputDebugString("envelope re-prime\n");
//}
//  }

//EMUTRACE2("[eprime %04X %04X]",env->reg_ad_x,env->reg_sr_x);

  env->level = 1;
  env->state = ENVELOPE_STATE_ATTACK;
  env->delta = 1;

  env->cachemax = 0;
}

static void EMU_CALL envelope_release(struct SPUCORE_ENVELOPE *env) {
  if(env->state != ENVELOPE_STATE_OFF) {
    env->state = ENVELOPE_STATE_RELEASE;
  }
  env->cachemax = 0;
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL sample_prime(struct SPUCORE_SAMPLE *sample) {
  sample->state = SAMPLE_STATE_ON;
  memset(sample->array, 0, sizeof(sample->array));
  sample->array_cleared = 1;
  sample->phase = 0x1C000; // ensure it grabs the first block right away
//EMUTRACE2("[sprime %08X %08X]", sample->start_block_addr, sample->start_loop_block_addr);

  sample->block_addr = sample->start_block_addr;
  //sample->loop_block_addr = sample->start_loop_block_addr;
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL voice_on(struct SPUCORE_CHAN *c) {

  // FOR DEBUGGING PURPOSES ONLY.
//  if(c->sample.state == SAMPLE_STATE_ON)return;
//  if(c->env.state != ENVELOPE_STATE_OFF)return;
  /*
  ** Defer if already on
  */
  if(c->env.state != ENVELOPE_STATE_OFF) {
    //EMUTRACE0("alreadyon:");
    if(!(c->samples_until_pending_keyon)) {
      //EMUTRACE0("defer");
      c->samples_until_pending_keyon = KEYON_DEFER_SAMPLES;
    } else {
      //EMUTRACE0("was already deferred");
    }
  } else {
//    EMUTRACE0("prime");
    sample_prime(&(c->sample));
    envelope_prime(&(c->env));
  }
//  EMUTRACE0("\n");
}

static void EMU_CALL voice_off(struct SPUCORE_CHAN *c) {
  //EMUTRACE0("release");
  envelope_release(&(c->env));
  //EMUTRACE0("\n");
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL voices_on(struct SPUCORE_STATE *state, uint32 bits) {
  int a;
  for(a = 0; a < 24; a++) {
    if(bits & 1) {

//      EMUTRACE1("voiceon  %2d:",a);

//EMUTRACE2("[vol %08X %08X]",state->chan[a].vol[0].level,state->chan[a].vol[1].level);
//EMUTRACE2("[mvol %08X %08X]",state->mvol[0].level,state->mvol[1].level);
////EMUTRACE2("[avol %08X %08X]",state->avol[0],state->avol[1]);
//EMUTRACE2("[evol %08X %08X]",state->evol[0],state->evol[1]);

//    sint32 extvol_l = ((sint16)(state->avol[0]));
//    sint32 extvol_r = ((sint16)(state->avol[1]));

      voice_on(state->chan + a);
    }
    bits >>= 1;
  }
}

static void EMU_CALL voices_off(struct SPUCORE_STATE *state, uint32 bits) {
  int a;
  for(a = 0; a < 24; a++) {
    if(bits & 1) {
      //EMUTRACE1("voiceoff %2d:",a);
      voice_off(state->chan + a);
    }
    bits >>= 1;
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** - Scales samples in a buffer using an envelope
** - Returns the actual number of samples modified
**
** All UNMODIFIED samples must then be discarded
** (they're supposed to be zero)
*/
static int EMU_CALL enveloper(
  struct SPUCORE_ENVELOPE *env,
  sint32 *buf,
  int samples
) {
  int i = 0;
  while(i < samples) {
    sint32 e, d, max;
    if(env->state == ENVELOPE_STATE_OFF) break;
    max = env->cachemax;
    if(!max) {
      max = envelope_do(env);
      env->cachemax = max;
    }
    e = env->level;
    d = env->delta;
    if(max < 1) max = 1;
    if(max > (samples - i)) { max = samples - i; }
    env->cachemax -= max;
    if(!buf) {
      i += max;
      e += max * d;
    } else {
      while(max--) {
        sint32 b = buf[i];
        b *= (e >> 16);
        b >>= 15;
        buf[i] = b;
        e += d;
        i++;
      }
    }
    env->level = e;
    env->delta = d;
  }
  return i;
}

////////////////////////////////////////////////////////////////////////////////
//
// - Calls the resampler and enveloper
// - Returns the number of samples actually generated
//   (can be 0 if the channel is silent)
//
// If buf is null, it means "fast forward" - don't render anything
//
//
// output of render_channel_raw() is guaranteed clipped as long as
// resampler() and enveloper() are also
//
static int EMU_CALL render_channel_raw(
  uint16 *ram,
  uint32 memmax,
  struct SPUCORE_CHAN *c,
  sint32 *buf,
  sint32 *fmbuf,
  sint32 *nbuf,
  int samples,
  struct SPUCORE_IRQ_STATE *irq_state
) {
  int r = samples;
  /* If the envelope is dead, don't bother anyway */
  if((c->env.state) == ENVELOPE_STATE_OFF) return 0;
  /* Do resampling */
  if (fmbuf) r = resampler_modulated(ram, memmax, &(c->sample), buf, r, c->voice_pitch, fmbuf, irq_state);
  else r = resampler(ram, memmax, &(c->sample), buf, r, c->voice_pitch, irq_state);
  if(nbuf) {
    r = samples;
    if(buf) memcpy(buf, nbuf, 4 * r);
  }
  /* Do enveloping */
  r = enveloper(&(c->env), buf, r);
  /* If we were cut short by _either_, then the envelope state must be set
  ** to OFF */
  if(r < samples) c->env.state = ENVELOPE_STATE_OFF;
  return r;
}

//
// This is the new render_channel which processes deferred key-ons
//
//
// output of render_channel_mono() is guaranteed clipped as long as
// render_channel_raw() is also
//
static int EMU_CALL render_channel_mono(
  uint16 *ram,
  uint32 memmax,
  struct SPUCORE_CHAN *c,
  sint32 *buf,
  sint32 *fmbuf,
  sint32 *nbuf,
  sint32 samples,
  struct SPUCORE_IRQ_STATE *irq_state
) {
  sint32 n;
  sint32 r, r2;
  sint32 defer_remaining;
  struct SPUCORE_IRQ_STATE spare_state;

//top:
  n = c->samples_until_pending_keyon;

  if(!n) {
    return render_channel_raw(ram, memmax, c, buf, fmbuf, nbuf, samples, irq_state);
  }

  //
  // Don't defer key-on if fast-forwarding
  // this caused a STUCK-NOTE BUG. should not use.
  //
//  if(!buf) {
//    c->samples_until_pending_keyon = 0;
//    goto top;
//  }

  if(n > samples) { n = samples; }

  /*
  ** r = how many samples we actually will process
  */
  r = render_channel_raw(ram, memmax, c, buf, fmbuf, nbuf, n, irq_state);

  defer_remaining = c->samples_until_pending_keyon;
  if(buf) {
    sint32 i;
    for(i = 0; i < r; i++) {
      (*buf) = ((*buf)*defer_remaining) / KEYON_DEFER_SAMPLES;
      buf++;
      defer_remaining--;
    }
  } else {
    defer_remaining -= r;
  }
  if(fmbuf) fmbuf += r;
  if(nbuf) nbuf += r;
  /*
  ** if render_channel_raw got cut short, then we're done anyway.
  */
  if(r < n) defer_remaining = 0;

  c->samples_until_pending_keyon = defer_remaining;
  /*
  ** Process the key-on if necessary
  */
  if(!defer_remaining) {
    sample_prime(&(c->sample));
    envelope_prime(&(c->env));
  }

  /*
  ** we already handled r samples
  */
  samples -= r;

  /*
  ** if there are any remaining samples to render, do them here
  */
  r2 = 0;
  if(samples) {
    struct SPUCORE_IRQ_STATE *s = NULL;
    if(irq_state) {
      s = &spare_state;
      spare_state.offset = irq_state->offset;
    }
    r2 = render_channel_raw(ram, memmax, c, buf, fmbuf, nbuf, samples, s);
	if(irq_state && irq_state->triggered_cycle == 0xFFFFFFFF && spare_state.triggered_cycle != 0xFFFFFFFF) irq_state->triggered_cycle = spare_state.triggered_cycle + r * 768;
  }

  return r + r2;

}

////////////////////////////////////////////////////////////////////////////////
//
// Generates a buffer worth of noise data, ganked from Eternal SPU
//

static void EMU_CALL render_noise(
  struct SPUCORE_STATE *state,
  sint32 *buf,
  sint32 samples
) {
  int n, o;
  uint32 noiseclock = state->noiseclock;
  uint32 noisecounter = state->noisecounter;
  sint32 noiseval = state->noiseval;
  uint32 noisetemp = (noiseclock & 3) + 4;
  uint32 noiserev;
  noiseclock = (noiseclock >> 2) + 2;
  noiseclock = noisetemp << noiseclock;
  for(n = 0; n < samples; n++) {
    for(o = 0, noiserev = 0, noisetemp = noisecounter & 0xFFFFF; o < 5; o++) {
      noiserev = (noiserev << 4) | bit_reverse_table[noisetemp & 0xF];
      noisetemp >>= 4;
    }
    if(noiserev < noiseclock) {
      noisetemp = 0;
      if (noiseval & (1 << 31)) noisetemp = 1;
      if (noiseval & (1 << 21)) noisetemp ^= 1;
      if (noiseval & (1 <<  1)) noisetemp ^= 1;
      if (noiseval & (1 <<  0)) noisetemp ^= 1;
      noiseval = (noiseval << 1) | noisetemp;
    }
    noisecounter++;
    if (buf) *buf++ = noiseval >> 16;
  }
  state->noisecounter = noisecounter;
  state->noiseval = noiseval;
}

////////////////////////////////////////////////////////////////////////////////

#define MAKE_SINT32_COEF(x)   ((sint32)((sint16)(state->reverb.x)))
#define MAKE_REVERB_OFFSET(x) (state->reverb.x)
#define NORMALIZE_REVERB_OFFSET(x) {while(x>=state->reverb.safe_end_address){x-=state->reverb.safe_size;}while(x<state->reverb.safe_start_address){x+=state->reverb.safe_size;}}

#define RAM_PCM_SAMPLE(x) (*((sint16*)(((uint8*)ram) + (x))))
#define RAM_SINT32_SAMPLE(x) ((sint32)(RAM_PCM_SAMPLE(x)))
/*
** Multiplying -32768 by -32768 and scaling by 15 bits is not safe
** (the sign would get flipped)
** so we should clip to -32767 instead
*/
#define CLIP_PCM_1(x) {if(x>32767){x=32767;}else if(x<(-32767)){x=(-32767);}}
#define CLIP_PCM_2(a,b) {CLIP_PCM_1(a);CLIP_PCM_1(b);}
#define CLIP_PCM_4(a,b,c,d) {CLIP_PCM_1(a);CLIP_PCM_1(b);CLIP_PCM_1(c);CLIP_PCM_1(d);}

//#define CLIP_PCMDBL_1(x) {if(x>(32767*2)){x=(32767*2);}else if(x<(-(32767*2))){x=(-(32767*2));}}
//#define CLIP_PCMDBL_2(a,b) {CLIP_PCMDBL_1(a);CLIP_PCMDBL_1(b);}
//#define CLIP_PCMDBL_4(a,b,c,d) {CLIP_PCMDBL_1(a);CLIP_PCMDBL_1(b);CLIP_PCMDBL_1(c);CLIP_PCMDBL_1(d);}

////////////////////////////////////////////////////////////////////////////////
/*
** 22KHz reverb steady state step
*/
static void EMU_CALL reverb_steadystate22(struct SPUCORE_STATE *state, uint16 *ram, sint32 input_l, sint32 input_r) {
  /*
  ** Current reverb offset
  */
  sint32 current     = state->reverb.current_address;
  /*
  ** Reverb registers
  */
  sint32 fb_src_a    = MAKE_REVERB_OFFSET(FB_SRC_A);
  sint32 fb_src_b    = MAKE_REVERB_OFFSET(FB_SRC_B);
  sint32 iir_alpha   = MAKE_SINT32_COEF(IIR_ALPHA);
  sint32 acc_coef_a  = MAKE_SINT32_COEF(ACC_COEF_A);
  sint32 acc_coef_b  = MAKE_SINT32_COEF(ACC_COEF_B);
  sint32 acc_coef_c  = MAKE_SINT32_COEF(ACC_COEF_C);
  sint32 acc_coef_d  = MAKE_SINT32_COEF(ACC_COEF_D);
  sint32 iir_coef    = MAKE_SINT32_COEF(IIR_COEF);
  sint32 fb_alpha    = MAKE_SINT32_COEF(FB_ALPHA);
  sint32 fb_x        = MAKE_SINT32_COEF(FB_X);
  sint32 iir_dest_a0 = MAKE_REVERB_OFFSET(IIR_DEST_A0);
  sint32 iir_dest_a1 = MAKE_REVERB_OFFSET(IIR_DEST_A1);
  sint32 acc_src_a0  = MAKE_REVERB_OFFSET(ACC_SRC_A0);
  sint32 acc_src_a1  = MAKE_REVERB_OFFSET(ACC_SRC_A1);
  sint32 acc_src_b0  = MAKE_REVERB_OFFSET(ACC_SRC_B0);
  sint32 acc_src_b1  = MAKE_REVERB_OFFSET(ACC_SRC_B1);
  sint32 iir_src_a0  = MAKE_REVERB_OFFSET(IIR_SRC_A0);
  sint32 iir_src_a1  = MAKE_REVERB_OFFSET(IIR_SRC_A1);
  sint32 iir_dest_b0 = MAKE_REVERB_OFFSET(IIR_DEST_B0);
  sint32 iir_dest_b1 = MAKE_REVERB_OFFSET(IIR_DEST_B1);
  sint32 acc_src_c0  = MAKE_REVERB_OFFSET(ACC_SRC_C0);
  sint32 acc_src_c1  = MAKE_REVERB_OFFSET(ACC_SRC_C1);
  sint32 acc_src_d0  = MAKE_REVERB_OFFSET(ACC_SRC_D0);
  sint32 acc_src_d1  = MAKE_REVERB_OFFSET(ACC_SRC_D1);
  sint32 iir_src_b1  = MAKE_REVERB_OFFSET(IIR_SRC_B1);
  sint32 iir_src_b0  = MAKE_REVERB_OFFSET(IIR_SRC_B0);
  sint32 mix_dest_a0 = MAKE_REVERB_OFFSET(MIX_DEST_A0);
  sint32 mix_dest_a1 = MAKE_REVERB_OFFSET(MIX_DEST_A1);
  sint32 mix_dest_b0 = MAKE_REVERB_OFFSET(MIX_DEST_B0);
  sint32 mix_dest_b1 = MAKE_REVERB_OFFSET(MIX_DEST_B1);
  sint32 in_coef_l   = MAKE_SINT32_COEF(IN_COEF_L);
  sint32 in_coef_r   = MAKE_SINT32_COEF(IN_COEF_R);
  /*
  ** Alternate buffer positions
  */
  sint32 fb_src_a0;
  sint32 fb_src_a1;
  sint32 fb_src_b0;
  sint32 fb_src_b1;
  sint32 iir_dest_a0_plus;
  sint32 iir_dest_a1_plus;
  sint32 iir_dest_b0_plus;
  sint32 iir_dest_b1_plus;
  /*
  ** Intermediate results
  */
  sint32 acc0;
  sint32 acc1;
  sint32 iir_input_a0;
  sint32 iir_input_a1;
  sint32 iir_input_b0;
  sint32 iir_input_b1;
  sint32 iir_a0;
  sint32 iir_a1;
  sint32 iir_b0;
  sint32 iir_b1;
  sint32 fb_a0;
  sint32 fb_a1;
  sint32 fb_b0;
  sint32 fb_b1;
  sint32 mix_a0;
  sint32 mix_a1;
  sint32 mix_b0;
  sint32 mix_b1;

  /*
  ** Offsets
  */
  mix_dest_a0 += current; NORMALIZE_REVERB_OFFSET(mix_dest_a0);
  mix_dest_a1 += current; NORMALIZE_REVERB_OFFSET(mix_dest_a1);
  mix_dest_b0 += current; NORMALIZE_REVERB_OFFSET(mix_dest_b0);
  mix_dest_b1 += current; NORMALIZE_REVERB_OFFSET(mix_dest_b1);
  fb_src_a0 = mix_dest_a0 - fb_src_a; NORMALIZE_REVERB_OFFSET(fb_src_a0);
  fb_src_a1 = mix_dest_a1 - fb_src_a; NORMALIZE_REVERB_OFFSET(fb_src_a1);
  fb_src_b0 = mix_dest_b0 - fb_src_b; NORMALIZE_REVERB_OFFSET(fb_src_b0);
  fb_src_b1 = mix_dest_b1 - fb_src_b; NORMALIZE_REVERB_OFFSET(fb_src_b1);
  acc_src_a0 += current; NORMALIZE_REVERB_OFFSET(acc_src_a0);
  acc_src_a1 += current; NORMALIZE_REVERB_OFFSET(acc_src_a1);
  acc_src_b0 += current; NORMALIZE_REVERB_OFFSET(acc_src_b0);
  acc_src_b1 += current; NORMALIZE_REVERB_OFFSET(acc_src_b1);
  acc_src_c0 += current; NORMALIZE_REVERB_OFFSET(acc_src_c0);
  acc_src_c1 += current; NORMALIZE_REVERB_OFFSET(acc_src_c1);
  acc_src_d0 += current; NORMALIZE_REVERB_OFFSET(acc_src_d0);
  acc_src_d1 += current; NORMALIZE_REVERB_OFFSET(acc_src_d1);
  iir_src_a0 += current; NORMALIZE_REVERB_OFFSET(iir_src_a0);
  iir_src_a1 += current; NORMALIZE_REVERB_OFFSET(iir_src_a1);
  iir_src_b0 += current; NORMALIZE_REVERB_OFFSET(iir_src_b0);
  iir_src_b1 += current; NORMALIZE_REVERB_OFFSET(iir_src_b1);
  iir_dest_a0 += current; NORMALIZE_REVERB_OFFSET(iir_dest_a0);
  iir_dest_a1 += current; NORMALIZE_REVERB_OFFSET(iir_dest_a1);
  iir_dest_b0 += current; NORMALIZE_REVERB_OFFSET(iir_dest_b0);
  iir_dest_b1 += current; NORMALIZE_REVERB_OFFSET(iir_dest_b1);
  iir_dest_a0_plus = iir_dest_a0 + 2; NORMALIZE_REVERB_OFFSET(iir_dest_a0_plus);
  iir_dest_a1_plus = iir_dest_a1 + 2; NORMALIZE_REVERB_OFFSET(iir_dest_a1_plus);
  iir_dest_b0_plus = iir_dest_b0 + 2; NORMALIZE_REVERB_OFFSET(iir_dest_b0_plus);
  iir_dest_b1_plus = iir_dest_b1 + 2; NORMALIZE_REVERB_OFFSET(iir_dest_b1_plus);

  /*
  ** IIR
  */
  CLIP_PCM_2(input_l,input_r);
  input_l *= in_coef_l;
  input_r *= in_coef_r;
#define OPPOSITE_IIR_ALPHA (32768-iir_alpha)
  iir_input_a0 = ((RAM_SINT32_SAMPLE(iir_src_a0) * iir_coef) + input_l) >> 15;
  iir_input_a1 = ((RAM_SINT32_SAMPLE(iir_src_a1) * iir_coef) + input_r) >> 15;
  iir_input_b0 = ((RAM_SINT32_SAMPLE(iir_src_b0) * iir_coef) + input_l) >> 15;
  iir_input_b1 = ((RAM_SINT32_SAMPLE(iir_src_b1) * iir_coef) + input_r) >> 15;
  CLIP_PCM_4(iir_input_a0,iir_input_a1,iir_input_b0,iir_input_b1);
  iir_a0 = ((iir_input_a0 * iir_alpha) + (RAM_SINT32_SAMPLE(iir_dest_a0) * (OPPOSITE_IIR_ALPHA))) >> 15;
  iir_a1 = ((iir_input_a1 * iir_alpha) + (RAM_SINT32_SAMPLE(iir_dest_a1) * (OPPOSITE_IIR_ALPHA))) >> 15;
  iir_b0 = ((iir_input_b0 * iir_alpha) + (RAM_SINT32_SAMPLE(iir_dest_b0) * (OPPOSITE_IIR_ALPHA))) >> 15;
  iir_b1 = ((iir_input_b1 * iir_alpha) + (RAM_SINT32_SAMPLE(iir_dest_b1) * (OPPOSITE_IIR_ALPHA))) >> 15;
  CLIP_PCM_4(iir_a0,iir_a1,iir_b0,iir_b1);

  RAM_PCM_SAMPLE(iir_dest_a0_plus) = iir_a0;
  RAM_PCM_SAMPLE(iir_dest_a1_plus) = iir_a1;
  RAM_PCM_SAMPLE(iir_dest_b0_plus) = iir_b0;
  RAM_PCM_SAMPLE(iir_dest_b1_plus) = iir_b1;

  /*
  ** Accumulators
  */
  acc0 =
    ((RAM_SINT32_SAMPLE(acc_src_a0) * acc_coef_a) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_b0) * acc_coef_b) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_c0) * acc_coef_c) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_d0) * acc_coef_d) >> 15);
  acc1 =
    ((RAM_SINT32_SAMPLE(acc_src_a1) * acc_coef_a) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_b1) * acc_coef_b) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_c1) * acc_coef_c) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_d1) * acc_coef_d) >> 15);
  CLIP_PCM_2(acc0,acc1);

  /*
  ** Feedback
  */
  fb_a0 = RAM_SINT32_SAMPLE(fb_src_a0);
  fb_a1 = RAM_SINT32_SAMPLE(fb_src_a1);
  fb_b0 = RAM_SINT32_SAMPLE(fb_src_b0);
  fb_b1 = RAM_SINT32_SAMPLE(fb_src_b1);

  mix_a0 = acc0 - ((fb_a0*fb_alpha)>>15);
  mix_a1 = acc1 - ((fb_a1*fb_alpha)>>15);
  mix_b0 = fb_alpha*acc0;
  mix_b1 = fb_alpha*acc1;
  fb_alpha = ((sint32)((sint16)(((sint16)fb_alpha)^0x8000)));
  mix_b0 -= fb_a0*fb_alpha;
  mix_b1 -= fb_a1*fb_alpha;
  mix_b0 -= fb_b0*fb_x;
  mix_b1 -= fb_b1*fb_x;
  mix_b0>>=15;
  mix_b1>>=15;

  CLIP_PCM_4(mix_a0,mix_a1,mix_b0,mix_b1);
  RAM_PCM_SAMPLE(mix_dest_a0) = mix_a0;
  RAM_PCM_SAMPLE(mix_dest_a1) = mix_a1;
  RAM_PCM_SAMPLE(mix_dest_b0) = mix_b0;
  RAM_PCM_SAMPLE(mix_dest_b1) = mix_b1;

}

////////////////////////////////////////////////////////////////////////////////
/*
** 22KHz reverb engine
*/
static void EMU_CALL reverb_engine22(struct SPUCORE_STATE *state, uint16 *ram, sint32 *l, sint32 *r) {
  sint32 input_l = *l;
  sint32 input_r = *r;
  sint32 output_l;
  sint32 output_r;
  sint32 mix_dest_a0 = state->reverb.current_address + MAKE_REVERB_OFFSET(MIX_DEST_A0);
  sint32 mix_dest_a1 = state->reverb.current_address + MAKE_REVERB_OFFSET(MIX_DEST_A1);
  sint32 mix_dest_b0 = state->reverb.current_address + MAKE_REVERB_OFFSET(MIX_DEST_B0);
  sint32 mix_dest_b1 = state->reverb.current_address + MAKE_REVERB_OFFSET(MIX_DEST_B1);
  NORMALIZE_REVERB_OFFSET(mix_dest_a0);
  NORMALIZE_REVERB_OFFSET(mix_dest_a1);
  NORMALIZE_REVERB_OFFSET(mix_dest_b0);
  NORMALIZE_REVERB_OFFSET(mix_dest_b1);

  /*
  ** (Scale these down for now - avoids some clipping)
  */
  input_l *= 2;
  input_r *= 2;
  input_l /= 3;
  input_r /= 3;

  /*
  ** Execute steady state step if necessary
  */
  if(state->flags & SPUREG_FLAG_REVERB_ENABLE) {
    reverb_steadystate22(state, ram, input_l, input_r);
  }

  /*
  ** Retrieve wet out L/R
  ** (pretty certain this is done AFTER the steady state step)
  */
  {
    int al = RAM_SINT32_SAMPLE(mix_dest_a0);
    int ar = RAM_SINT32_SAMPLE(mix_dest_a1);
    int bl = RAM_SINT32_SAMPLE(mix_dest_b0);
    int br = RAM_SINT32_SAMPLE(mix_dest_b1);

    output_l = al + bl;
    output_r = ar + br;
  }

  *l = output_l;
  *r = output_r;

  /*
  ** Advance reverb buffer position
  */
  state->reverb.current_address += 2;
  if(state->reverb.current_address >= state->reverb.safe_end_address) {
    state->reverb.current_address = state->reverb.safe_start_address;
  }
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL reverb_process(struct SPUCORE_STATE *state, uint16 *ram, sint32 *buf, int samples) {
  int q = state->reverb.resampler.queue_index;
  /*
  ** Sample loop
  */
  while(samples--) {
    /*
    ** Get an input sample
    */
    sint32 l = buf[0];
    sint32 r = buf[1];
    /*
    ** Put it in the input queue
    */
    state->reverb.resampler.in_queue_l[q & 7] = l;
    state->reverb.resampler.in_queue_r[q & 7] = r;
    /*
    ** If we're ready to create another output sample...
    */
    if(q & 1) {
      /*
      ** Lowpass/downsample
      */

      l =
        (state->reverb.resampler.in_queue_l[(q - 7) & 7]) * reverb_lowpass_coefs[0] +
        (state->reverb.resampler.in_queue_l[(q - 6) & 7]) * reverb_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_l[(q - 5) & 7]) * reverb_lowpass_coefs[2] +
        (state->reverb.resampler.in_queue_l[(q - 4) & 7]) * reverb_lowpass_coefs[3] +
        (state->reverb.resampler.in_queue_l[(q - 3) & 7]) * reverb_lowpass_coefs[4] +
        (state->reverb.resampler.in_queue_l[(q - 2) & 7]) * reverb_lowpass_coefs[5] +
        (state->reverb.resampler.in_queue_l[(q - 1) & 7]) * reverb_lowpass_coefs[6] +
        (state->reverb.resampler.in_queue_l[(q - 0) & 7]) * reverb_lowpass_coefs[7];
      l >>= 11;
      r =
        (state->reverb.resampler.in_queue_r[(q - 7) & 7]) * reverb_lowpass_coefs[0] +
        (state->reverb.resampler.in_queue_r[(q - 6) & 7]) * reverb_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_r[(q - 5) & 7]) * reverb_lowpass_coefs[2] +
        (state->reverb.resampler.in_queue_r[(q - 4) & 7]) * reverb_lowpass_coefs[3] +
        (state->reverb.resampler.in_queue_r[(q - 3) & 7]) * reverb_lowpass_coefs[4] +
        (state->reverb.resampler.in_queue_r[(q - 2) & 7]) * reverb_lowpass_coefs[5] +
        (state->reverb.resampler.in_queue_r[(q - 1) & 7]) * reverb_lowpass_coefs[6] +
        (state->reverb.resampler.in_queue_r[(q - 0) & 7]) * reverb_lowpass_coefs[7];
      r >>= 11;

/*
      l =
        (state->reverb.resampler.in_queue_l[(q - 6) & 7]) * reverb_new_lowpass_coefs[0] +
        (state->reverb.resampler.in_queue_l[(q - 4) & 7]) * reverb_new_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_l[(q - 3) & 7]) * reverb_new_lowpass_coefs[2] +
        (state->reverb.resampler.in_queue_l[(q - 2) & 7]) * reverb_new_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_l[(q - 0) & 7]) * reverb_new_lowpass_coefs[0];
      l >>= 11;
      r =
        (state->reverb.resampler.in_queue_r[(q - 6) & 7]) * reverb_new_lowpass_coefs[0] +
        (state->reverb.resampler.in_queue_r[(q - 4) & 7]) * reverb_new_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_r[(q - 3) & 7]) * reverb_new_lowpass_coefs[2] +
        (state->reverb.resampler.in_queue_r[(q - 2) & 7]) * reverb_new_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_r[(q - 0) & 7]) * reverb_new_lowpass_coefs[0];
      r >>= 11;
*/
//l = state->reverb.resampler.in_queue_l[q & 7];
//r = state->reverb.resampler.in_queue_r[q & 7];

      /*
      ** Run the reverb engine
      */
      reverb_engine22(state, ram, &l, &r);
      /*
      ** Put the new stuff into the output queue
      */
      state->reverb.resampler.out_queue_l[q & 15] = l;
      state->reverb.resampler.out_queue_r[q & 15] = r;
    }
    /*
    ** Upsample
    */
    /*
    ** Upsample using the same technique as for ADPCM samples
    ** (may or may not be right, sounds okay though)
    */
#define gauss_table_0x000 gauss_shuffled_reverse_table[0x200]
#define gauss_table_0x100 gauss_shuffled_reverse_table[0x201]
#define gauss_table_0x200 gauss_shuffled_reverse_table[0x202]
#define gauss_table_0x300 gauss_shuffled_reverse_table[0x203]
#define gauss_table_0x080 gauss_shuffled_reverse_table[0x000]
#define gauss_table_0x180 gauss_shuffled_reverse_table[0x001]
#define gauss_table_0x280 gauss_shuffled_reverse_table[0x002]
#define gauss_table_0x380 gauss_shuffled_reverse_table[0x003]
    if(q & 1) {
      l =
        (state->reverb.resampler.out_queue_l[(q - 6) & 15]) * gauss_table_0x080 +
        (state->reverb.resampler.out_queue_l[(q - 4) & 15]) * gauss_table_0x180 +
        (state->reverb.resampler.out_queue_l[(q - 2) & 15]) * gauss_table_0x280 +
        (state->reverb.resampler.out_queue_l[(q - 0) & 15]) * gauss_table_0x380;
      l >>= 11;
      r =
        (state->reverb.resampler.out_queue_r[(q - 6) & 15]) * gauss_table_0x080 +
        (state->reverb.resampler.out_queue_r[(q - 4) & 15]) * gauss_table_0x180 +
        (state->reverb.resampler.out_queue_r[(q - 2) & 15]) * gauss_table_0x280 +
        (state->reverb.resampler.out_queue_r[(q - 0) & 15]) * gauss_table_0x380;
      r >>= 11;
    } else {
      l =
        (state->reverb.resampler.out_queue_l[(q - 7) & 15]) * gauss_table_0x000 +
        (state->reverb.resampler.out_queue_l[(q - 5) & 15]) * gauss_table_0x100 +
        (state->reverb.resampler.out_queue_l[(q - 3) & 15]) * gauss_table_0x200 +
        (state->reverb.resampler.out_queue_l[(q - 1) & 15]) * gauss_table_0x300;
      l >>= 11;
      r =
        (state->reverb.resampler.out_queue_r[(q - 7) & 15]) * gauss_table_0x000 +
        (state->reverb.resampler.out_queue_r[(q - 5) & 15]) * gauss_table_0x100 +
        (state->reverb.resampler.out_queue_r[(q - 3) & 15]) * gauss_table_0x200 +
        (state->reverb.resampler.out_queue_r[(q - 1) & 15]) * gauss_table_0x300;
      r >>= 11;
    }
    /*
    ** Advance queue position, write output, all that good stuff
    */
    q++;
    buf[0] = l;
    buf[1] = r;
    buf += 2;
  }
  state->reverb.resampler.queue_index = q;
}

////////////////////////////////////////////////////////////////////////////////

//int spucore_frq[RENDERMAX];

/*
** Renderer
*/
static void EMU_CALL render(struct SPUCORE_STATE *state, uint16 *ram, sint16 *buf, sint16 *extinput, sint32 samples, uint8 mainout, uint8 effectout) {
  uint32 chanbit;
  uint32 maskmain_l;
  uint32 maskmain_r;
  uint32 maskverb_l;
  uint32 maskverb_r;
  uint32 masknoise;
  uint32 maskfm;
  sint32 ibuf   [  RENDERMAX];
  sint32 ibufmix[2*RENDERMAX];
  sint32 ibufrvb[2*RENDERMAX];
  sint32 ibufn  [  RENDERMAX];
  sint32 ibuffm [  RENDERMAX];
  int ch, i;
  sint32 m_v_l;
  sint32 m_v_r;
  sint32 r_v_l;
  sint32 r_v_r;
  struct SPUCORE_IRQ_STATE irq_state;
  struct SPUCORE_IRQ_STATE *irq_state_ptr;

  //spucore_frq[samples]++;

  irq_state_ptr = NULL;
  irq_state.triggered_cycle = 0xFFFFFFFF;
  if (state->flags & SPUREG_FLAG_IRQ_ENABLE) {
    if (state->memsize == 0x80000 && state->irq_address < 0x1000) {
      uint32 irq_address_masked = state->irq_address & 0x3FF;
      uint32 irq_sample_offset = irq_address_masked - state->irq_decoder_clock;
      if(irq_sample_offset > 0x3FF) irq_sample_offset += 0x400;
      if (irq_sample_offset < (uint32)samples) {
        irq_state.triggered_cycle = irq_sample_offset * 768;
      }
    } else {
      irq_state_ptr = &irq_state;
      irq_state.offset = state->irq_address;
    }
  }

  state->irq_decoder_clock = (state->irq_decoder_clock + samples) & 0x3FF;

  memset(ibufmix, 0, 8 * samples);
  if(effectout) {
    memset(ibufrvb, 0, 8 * samples);
  }

  maskmain_l = 0;
  maskmain_r = 0;
  maskverb_l = 0;
  maskverb_r = 0;
  if(state->flags & SPUREG_FLAG_MSNDL) maskmain_l = state->vmix[0] & 0xFFFFFF;
  if(state->flags & SPUREG_FLAG_MSNDR) maskmain_r = state->vmix[1] & 0xFFFFFF;
  if(effectout) {
    if(state->flags & SPUREG_FLAG_MSNDEL) maskverb_l = state->vmixe[0] & 0xFFFFFF;
    if(state->flags & SPUREG_FLAG_MSNDER) maskverb_r = state->vmixe[1] & 0xFFFFFF;
  }
  masknoise = state->noise;
  render_noise(state, masknoise ? ibufn : NULL, samples);
  maskfm = state->fm & 0xFFFFFE;

  if(!mainout) { maskmain_l = 0; maskmain_r = 0; }

  for(ch = 0, chanbit = 1; ch < 24; ch++, chanbit <<= 1) {
    int r;
    sint32 v_l, v_r;
    uint32 main_l = chanbit & maskmain_l;
    uint32 main_r = chanbit & maskmain_r;
    uint32 verb_l = chanbit & maskverb_l;
    uint32 verb_r = chanbit & maskverb_r;
    sint32 *b = buf ? ibuf : NULL;
    sint32 *fm = (chanbit & maskfm) ? ibuffm : NULL;
    sint32 *noise = (chanbit & masknoise) ? ibufn : NULL;
    if(!(main_l | main_r | verb_l | verb_r)) b = NULL;
    r = render_channel_mono(
      ram, state->memsize, state->chan + ch, b, fm, noise, samples, irq_state_ptr
    );
    if(!b) {
      memset(ibuffm, 0, 4 * samples);
      continue;
    }
    memcpy(ibuffm, ibuf, 4 * r);
    if(r < samples) memset(ibuffm + r, 0, 4 * (samples-r));
    v_l = volume_getlevel(state->chan[ch].vol+0);
    v_r = volume_getlevel(state->chan[ch].vol+1);
    for(i = 0; i < r; i++) {
      sint32 q_l = (v_l * ibuf[i]) >> 16;
      sint32 q_r = (v_r * ibuf[i]) >> 16;
      if(main_l) ibufmix[2*i+0] += q_l;
      if(main_r) ibufmix[2*i+1] += q_r;
      if(verb_l) ibufrvb[2*i+0] += q_l;
      if(verb_r) ibufrvb[2*i+1] += q_r;
    }
  }

  state->irq_triggered_cycle = irq_state.triggered_cycle;

  if(!buf) return;

  /*
  ** Mix in external input, if it exists
  */
  if(extinput) {
    sint32 extvol_l = ((sint16)(state->avol[0]));
    sint32 extvol_r = ((sint16)(state->avol[1]));
    if(extvol_l == -0x8000) extvol_l = -0x7FFF;
    if(extvol_r == -0x8000) extvol_r = -0x7FFF;
    for(i = 0; i < samples; i++) {
      sint32 sin_l = extinput[2*i+0];
      sint32 sin_r = extinput[2*i+1];
      sin_l *= extvol_l;
      sin_r *= extvol_r;
      sin_l >>= 15;
      sin_r >>= 15;
      if(state->flags & SPUREG_FLAG_SINL ) ibufmix[2*i+0] += sin_l;
      if(state->flags & SPUREG_FLAG_SINR ) ibufmix[2*i+1] += sin_r;
      if(state->flags & SPUREG_FLAG_SINEL) ibufrvb[2*i+0] += sin_l;
      if(state->flags & SPUREG_FLAG_SINER) ibufrvb[2*i+1] += sin_r;
    }
  }

  /*
  ** Do reverb
  ** This handles both writing into the buffer and retrieving
  ** values out of it, resampling to/from 22KHz, etc.
  */
  if(effectout) {
    reverb_process(state, ram, ibufrvb, samples);
  }
  /*
  **
  */
  m_v_l = volume_getlevel(state->mvol+0);
  m_v_r = volume_getlevel(state->mvol+1);
  r_v_l = ((sint16)(state->evol[0]));
  r_v_r = ((sint16)(state->evol[1]));

  if(!effectout) {
    for(i = 0; i < samples; i++) {
      sint64 q_l = ibufmix[2*i+0];
      sint64 q_r = ibufmix[2*i+1];
      q_l *= (sint64)m_v_l;
      q_r *= (sint64)m_v_r;
      q_l >>= 15;
      q_r >>= 15;
      CLIP_PCM_2(q_l,q_r);
      *buf++ = (sint16)q_l;
      *buf++ = (sint16)q_r;
    }
  } else {
    for(i = 0; i < samples; i++) {
      sint64 q_l = ibufmix[2*i+0];
      sint64 q_r = ibufmix[2*i+1];
      sint64 r_l = ibufrvb[2*i+0];
      sint64 r_r = ibufrvb[2*i+1];
      q_l *= (sint64)m_v_l;
      q_r *= (sint64)m_v_r;
      r_l *= (sint64)r_v_l;
      r_r *= (sint64)r_v_r;
      q_l >>= 15;
      q_r >>= 15;
      r_l >>= 15;
      r_r >>= 15;
      q_l += r_l;
      q_r += r_r;
      CLIP_PCM_2(q_l, q_r);
      *buf++ = (sint16)q_l;
      *buf++ = (sint16)q_r;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** Externally-accessible renderer
*/
void EMU_CALL spucore_render(void *state, uint16 *ram, sint16 *buf, sint16 *extinput, uint32 samples, uint8 mainout, uint8 effectout) {
  while(samples > RENDERMAX) {
    samples -= RENDERMAX;
    render(SPUCORESTATE, ram, buf, extinput, RENDERMAX, mainout, effectout);
    if(buf     ) buf      += 2 * RENDERMAX;
    if(extinput) extinput += 2 * RENDERMAX;
  }
  if(samples) render(SPUCORESTATE, ram, buf, extinput, samples, mainout, effectout);
}

////////////////////////////////////////////////////////////////////////////////
/*
** Flag get/set
*/

int EMU_CALL spucore_getflag(void *state, uint32 n) {
  return !!(SPUCORESTATE->flags & n);
}

void EMU_CALL spucore_setflag(void *state, uint32 n, int value) {
  if(value) {
    SPUCORESTATE->flags |= n;
  } else {
    SPUCORESTATE->flags &= ~n;
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** Register get/set
*/

uint32 EMU_CALL spucore_getreg(void *state, uint32 n) {
  switch(n) {
  case SPUREG_MVOLL:  return volume_getmode (SPUCORESTATE->mvol+0) & 0x0000FFFF;
  case SPUREG_MVOLR:  return volume_getmode (SPUCORESTATE->mvol+1) & 0x0000FFFF;
  case SPUREG_MVOLXL: return volume_getlevel(SPUCORESTATE->mvol+0) & 0x0000FFFF;
  case SPUREG_MVOLXR: return volume_getlevel(SPUCORESTATE->mvol+1) & 0x0000FFFF;
  case SPUREG_EVOLL:  return SPUCORESTATE->evol[0] & 0x0000FFFF;
  case SPUREG_EVOLR:  return SPUCORESTATE->evol[1] & 0x0000FFFF;
  case SPUREG_AVOLL:  return SPUCORESTATE->avol[0] & 0x0000FFFF;
  case SPUREG_AVOLR:  return SPUCORESTATE->avol[1] & 0x0000FFFF;
  case SPUREG_BVOLL:  return SPUCORESTATE->bvol[0] & 0x0000FFFF;
  case SPUREG_BVOLR:  return SPUCORESTATE->bvol[1] & 0x0000FFFF;
  case SPUREG_KON:    return SPUCORESTATE->kon     & 0x00FFFFFF;
  case SPUREG_KOFF:   return SPUCORESTATE->koff    & 0x00FFFFFF;
  case SPUREG_FM:     return SPUCORESTATE->fm      & 0x00FFFFFF;
  case SPUREG_NOISE:  return SPUCORESTATE->noise   & 0x00FFFFFF;
  case SPUREG_VMIXE:  return (SPUCORESTATE->vmixe[0] | SPUCORESTATE->vmixe[1]) & 0xFFFFFF;
  case SPUREG_VMIX:   return (SPUCORESTATE->vmix[0]  | SPUCORESTATE->vmix[1] ) & 0xFFFFFF;
  case SPUREG_VMIXEL: return SPUCORESTATE->vmixe[0] & 0x00FFFFFF;
  case SPUREG_VMIXER: return SPUCORESTATE->vmixe[1] & 0x00FFFFFF;
  case SPUREG_VMIXL:  return SPUCORESTATE->vmix[0]  & 0x00FFFFFF;
  case SPUREG_VMIXR:  return SPUCORESTATE->vmix[1]  & 0x00FFFFFF;
  case SPUREG_ESA:    return SPUCORESTATE->reverb.start_address;
  case SPUREG_EEA:    return SPUCORESTATE->reverb.end_address;
  case SPUREG_EAX:    return SPUCORESTATE->reverb.current_address;
  case SPUREG_IRQA:   return SPUCORESTATE->irq_address;
  case SPUREG_NOISECLOCK: return SPUCORESTATE->noiseclock;
#define SPUCORE_REVERB_GET(x) case SPUREG_REVERB_##x:return SPUCORESTATE->reverb.x;
  SPUCORE_REVERB_GET(FB_SRC_A)
  SPUCORE_REVERB_GET(FB_SRC_B)
  SPUCORE_REVERB_GET(IIR_ALPHA)
  SPUCORE_REVERB_GET(ACC_COEF_A)
  SPUCORE_REVERB_GET(ACC_COEF_B)
  SPUCORE_REVERB_GET(ACC_COEF_C)
  SPUCORE_REVERB_GET(ACC_COEF_D)
  SPUCORE_REVERB_GET(IIR_COEF)
  SPUCORE_REVERB_GET(FB_ALPHA)
  SPUCORE_REVERB_GET(FB_X)
  SPUCORE_REVERB_GET(IIR_DEST_A0)
  SPUCORE_REVERB_GET(IIR_DEST_A1)
  SPUCORE_REVERB_GET(ACC_SRC_A0)
  SPUCORE_REVERB_GET(ACC_SRC_A1)
  SPUCORE_REVERB_GET(ACC_SRC_B0)
  SPUCORE_REVERB_GET(ACC_SRC_B1)
  SPUCORE_REVERB_GET(IIR_SRC_A0)
  SPUCORE_REVERB_GET(IIR_SRC_A1)
  SPUCORE_REVERB_GET(IIR_DEST_B0)
  SPUCORE_REVERB_GET(IIR_DEST_B1)
  SPUCORE_REVERB_GET(ACC_SRC_C0)
  SPUCORE_REVERB_GET(ACC_SRC_C1)
  SPUCORE_REVERB_GET(ACC_SRC_D0)
  SPUCORE_REVERB_GET(ACC_SRC_D1)
  SPUCORE_REVERB_GET(IIR_SRC_B1)
  SPUCORE_REVERB_GET(IIR_SRC_B0)
  SPUCORE_REVERB_GET(MIX_DEST_A0)
  SPUCORE_REVERB_GET(MIX_DEST_A1)
  SPUCORE_REVERB_GET(MIX_DEST_B0)
  SPUCORE_REVERB_GET(MIX_DEST_B1)
  SPUCORE_REVERB_GET(IN_COEF_L)
  SPUCORE_REVERB_GET(IN_COEF_R)
  }
  return 0;
}

void EMU_CALL spucore_setreg(void *state, uint32 n, uint32 value, uint32 mask) {
  value &= mask;
  switch(n) {
  /* TODO: the increase/decrease modes, etc. */
  case SPUREG_MVOLL: volume_setmode(SPUCORESTATE->mvol+0, value); break;
  case SPUREG_MVOLR: volume_setmode(SPUCORESTATE->mvol+1, value); break;
  case SPUREG_EVOLL: SPUCORESTATE->evol[0] = value; break;
  case SPUREG_EVOLR: SPUCORESTATE->evol[1] = value; break;
  case SPUREG_AVOLL: SPUCORESTATE->avol[0] = value; break;
  case SPUREG_AVOLR: SPUCORESTATE->avol[1] = value; break;
  case SPUREG_BVOLL: SPUCORESTATE->bvol[0] = value; break;
  case SPUREG_BVOLR: SPUCORESTATE->bvol[1] = value; break;

  case SPUREG_KON:
    SPUCORESTATE->kon &= ~mask;
    SPUCORESTATE->kon |= value;
    voices_on(state, value);
    break;
  case SPUREG_KOFF:
    SPUCORESTATE->koff &= ~mask;
    SPUCORESTATE->koff |= value;
    voices_off(state, value);
    break;

  case SPUREG_FM:
    SPUCORESTATE->fm &= ~mask;
    SPUCORESTATE->fm |= value;
    break;
  case SPUREG_NOISE:
    SPUCORESTATE->noise &= ~mask;
    SPUCORESTATE->noise |= value;
    break;
  case SPUREG_VMIXE:
    SPUCORESTATE->vmixe[0] &= ~mask;
    SPUCORESTATE->vmixe[0] |= value;
    SPUCORESTATE->vmixe[1] &= ~mask;
    SPUCORESTATE->vmixe[1] |= value;
    break;
  case SPUREG_VMIX:
    SPUCORESTATE->vmix[0] &= ~mask;
    SPUCORESTATE->vmix[0] |= value;
    SPUCORESTATE->vmix[1] &= ~mask;
    SPUCORESTATE->vmix[1] |= value;
    break;
  case SPUREG_VMIXEL:
    SPUCORESTATE->vmixe[0] &= ~mask;
    SPUCORESTATE->vmixe[0] |= value;
    break;
  case SPUREG_VMIXER:
    SPUCORESTATE->vmixe[1] &= ~mask;
    SPUCORESTATE->vmixe[1] |= value;
    break;
  case SPUREG_VMIXL:
    SPUCORESTATE->vmix[0] &= ~mask;
    SPUCORESTATE->vmix[0] |= value;
    break;
  case SPUREG_VMIXR:
    SPUCORESTATE->vmix[1] &= ~mask;
    SPUCORESTATE->vmix[1] |= value;
    break;

  case SPUREG_ESA:
    SPUCORESTATE->reverb.start_address &= ~mask;
    SPUCORESTATE->reverb.start_address |= value;
    make_safe_reverb_addresses(state);
    SPUCORESTATE->reverb.current_address = SPUCORESTATE->reverb.safe_start_address;
    break;
  case SPUREG_EEA:
    SPUCORESTATE->reverb.end_address &= ~mask;
    SPUCORESTATE->reverb.end_address |= value;
    make_safe_reverb_addresses(state);
    break;
  case SPUREG_IRQA:
    /* TODO: actual implementation of IRQs */
    SPUCORESTATE->irq_address &= ~mask;
    SPUCORESTATE->irq_address |= value;
    break;
  case SPUREG_NOISECLOCK:
    SPUCORESTATE->noiseclock = value & 0x3F;
    break;

#define SPUCORE_REVERB_SET(x) case SPUREG_REVERB_##x:SPUCORESTATE->reverb.x&=(~mask);SPUCORESTATE->reverb.x|=value;break;
  SPUCORE_REVERB_SET(FB_SRC_A)
  SPUCORE_REVERB_SET(FB_SRC_B)
  SPUCORE_REVERB_SET(IIR_ALPHA)
  SPUCORE_REVERB_SET(ACC_COEF_A)
  SPUCORE_REVERB_SET(ACC_COEF_B)
  SPUCORE_REVERB_SET(ACC_COEF_C)
  SPUCORE_REVERB_SET(ACC_COEF_D)
  SPUCORE_REVERB_SET(IIR_COEF)
  SPUCORE_REVERB_SET(FB_ALPHA)
  SPUCORE_REVERB_SET(FB_X)
  SPUCORE_REVERB_SET(IIR_DEST_A0)
  SPUCORE_REVERB_SET(IIR_DEST_A1)
  SPUCORE_REVERB_SET(ACC_SRC_A0)
  SPUCORE_REVERB_SET(ACC_SRC_A1)
  SPUCORE_REVERB_SET(ACC_SRC_B0)
  SPUCORE_REVERB_SET(ACC_SRC_B1)
  SPUCORE_REVERB_SET(IIR_SRC_A0)
  SPUCORE_REVERB_SET(IIR_SRC_A1)
  SPUCORE_REVERB_SET(IIR_DEST_B0)
  SPUCORE_REVERB_SET(IIR_DEST_B1)
  SPUCORE_REVERB_SET(ACC_SRC_C0)
  SPUCORE_REVERB_SET(ACC_SRC_C1)
  SPUCORE_REVERB_SET(ACC_SRC_D0)
  SPUCORE_REVERB_SET(ACC_SRC_D1)
  SPUCORE_REVERB_SET(IIR_SRC_B1)
  SPUCORE_REVERB_SET(IIR_SRC_B0)
  SPUCORE_REVERB_SET(MIX_DEST_A0)
  SPUCORE_REVERB_SET(MIX_DEST_A1)
  SPUCORE_REVERB_SET(MIX_DEST_B0)
  SPUCORE_REVERB_SET(MIX_DEST_B1)
  SPUCORE_REVERB_SET(IN_COEF_L)
  SPUCORE_REVERB_SET(IN_COEF_R)
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** Voice register get/set
*/

uint32 EMU_CALL spucore_getreg_voice(void *state, uint32 voice, uint32 n) {
  switch(n) {
  case SPUREG_VOICE_VOLL : return volume_getmode(SPUCORESTATE->chan[voice].vol+0);
  case SPUREG_VOICE_VOLR : return volume_getmode(SPUCORESTATE->chan[voice].vol+1);
  case SPUREG_VOICE_VOLXL: return volume_getlevel(SPUCORESTATE->chan[voice].vol+0);
  case SPUREG_VOICE_VOLXR: return volume_getlevel(SPUCORESTATE->chan[voice].vol+1);
  case SPUREG_VOICE_PITCH: return SPUCORESTATE->chan[voice].voice_pitch;
  case SPUREG_VOICE_ADSR1: return SPUCORESTATE->chan[voice].env.reg_ad;
  case SPUREG_VOICE_ADSR2: return SPUCORESTATE->chan[voice].env.reg_sr;
  case SPUREG_VOICE_ENVX :
    if(SPUCORESTATE->chan[voice].env.state == ENVELOPE_STATE_OFF) return 0;
    return (SPUCORESTATE->chan[voice].env.level) >> 16;
  case SPUREG_VOICE_SSA  : return SPUCORESTATE->chan[voice].sample.start_block_addr;
  case SPUREG_VOICE_LSAX : return SPUCORESTATE->chan[voice].sample.loop_block_addr;
  case SPUREG_VOICE_NAX  : return SPUCORESTATE->chan[voice].sample.block_addr;
  }
  return 0;
}

void EMU_CALL spucore_setreg_voice(void *state, uint32 voice, uint32 n, uint32 value, uint32 mask) {
  value &= mask;
  switch(n) {
  /* TODO: the increase/decrease modes */
  case SPUREG_VOICE_VOLL : volume_setmode(SPUCORESTATE->chan[voice].vol+0, value); break;
  case SPUREG_VOICE_VOLR : volume_setmode(SPUCORESTATE->chan[voice].vol+1, value); break;
  case SPUREG_VOICE_PITCH: SPUCORESTATE->chan[voice].voice_pitch = value; break;
  case SPUREG_VOICE_ADSR1: SPUCORESTATE->chan[voice].env.reg_ad = value; SPUCORESTATE->chan[voice].env.cachemax = envelope_do(&SPUCORESTATE->chan[voice].env); break;
  case SPUREG_VOICE_ADSR2: SPUCORESTATE->chan[voice].env.reg_sr = value; SPUCORESTATE->chan[voice].env.cachemax = envelope_do(&SPUCORESTATE->chan[voice].env); break;

  case SPUREG_VOICE_SSA:
    SPUCORESTATE->chan[voice].sample.start_block_addr &= ~mask;
    SPUCORESTATE->chan[voice].sample.start_block_addr |= value;
    break;
  case SPUREG_VOICE_LSAX:
    SPUCORESTATE->chan[voice].sample.loop_block_addr &= ~mask;
    SPUCORESTATE->chan[voice].sample.loop_block_addr |= value;
    break;
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** IRQ checking
*/
uint32 EMU_CALL spucore_cycles_until_interrupt(void *state, uint16 *ram, uint32 samples) {
  uint32 r;
  void *backup;

  if (!(SPUCORESTATE->flags & SPUREG_FLAG_IRQ_ENABLE)) return 0xFFFFFFFF;

  backup = malloc(spucore_get_state_size());
  if (!backup) return 0xFFFFFFFF;
  memcpy(backup, state, spucore_get_state_size());
  state = backup;
  SPUCORESTATE->irq_triggered_cycle = 0xFFFFFFFF;
  r = 0;
  while(samples > RENDERMAX) {
    samples -= RENDERMAX;
    render(SPUCORESTATE, ram, NULL, NULL, RENDERMAX, 0, 0);
	if (SPUCORESTATE->irq_triggered_cycle != 0xFFFFFFFF) break;
	r += RENDERMAX * 768;
  }
  if(samples && SPUCORESTATE->irq_triggered_cycle == 0xFFFFFFFF) render(SPUCORESTATE, ram, NULL, NULL, samples, 0, 0);
  r = (SPUCORESTATE->irq_triggered_cycle == 0xFFFFFFFF) ? 0xFFFFFFFF : SPUCORESTATE->irq_triggered_cycle + r;
  free(backup);
  return r;
}

////////////////////////////////////////////////////////////////////////////////
