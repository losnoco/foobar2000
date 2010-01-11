/* C code produced by gperf version 3.0.1 */
/* Command-line: gperf -tCcTonD -K name -N frame_lookup --ignore-case id3_fid_reverse.gperf  */
/* Computed positions: -k'1,4,9' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "id3_fid_reverse.gperf"


#include <string.h>

#define ID3LIB_LINKOPTION 1
#include <id3.h>

struct id3_frame {
	char const * name;
	ID3_FrameID id;
};


#define TOTAL_KEYWORDS 52
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 17
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 135
/* maximum key range = 134, duplicates = 0 */

#ifndef GPERF_DOWNCASE
#define GPERF_DOWNCASE 1
static unsigned char gperf_downcase[256] =
  {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
     45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255
  };
#endif

#ifndef GPERF_CASE_STRNCMP
#define GPERF_CASE_STRNCMP 1
static int
gperf_case_strncmp (s1, s2, n)
     register const char *s1;
     register const char *s2;
     register unsigned int n;
{
  for (; n > 0;)
    {
      unsigned char c1 = gperf_downcase[(unsigned char)*s1++];
      unsigned char c2 = gperf_downcase[(unsigned char)*s2++];
      if (c1 != 0 && c1 == c2)
        {
          n--;
          continue;
        }
      return (int)c1 - (int)c2;
    }
  return 0;
}
#endif

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned char asso_values[] =
    {
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136,  16,   2,  20,   6,   0,
       61,  15, 136,  40, 136, 136,   1,  22,   2,  15,
       45,  25,   5,  10,  45,  21, 136,  55, 136,   6,
      136, 136, 136, 136, 136, 136, 136,  16,   2,  20,
        6,   0,  61,  15, 136,  40, 136, 136,   1,  22,
        2,  15,  45,  25,   5,  10,  45,  21, 136,  55,
      136,   6, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
      136, 136, 136, 136, 136, 136
    };
  register int hval = 0;

  switch (len)
    {
      default:
        hval += asso_values[(unsigned char)str[8]];
      /*FALLTHROUGH*/
      case 8:
      case 7:
      case 6:
      case 5:
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#endif
const struct id3_frame *
frame_lookup (str, len)
     register const char *str;
     register unsigned int len;
{
  static const struct id3_frame wordlist[] =
    {
#line 17 "id3_fid_reverse.gperf"
      {"BPM",				ID3FID_BPM},
#line 21 "id3_fid_reverse.gperf"
      {"DATE",				ID3FID_DATE},
#line 41 "id3_fid_reverse.gperf"
      {"BAND",				ID3FID_BAND},
#line 50 "id3_fid_reverse.gperf"
      {"SIZE",				ID3FID_SIZE},
#line 53 "id3_fid_reverse.gperf"
      {"YEAR",				ID3FID_YEAR},
#line 52 "id3_fid_reverse.gperf"
      {"ENCODERSETTINGS",	ID3FID_ENCODERSETTINGS},
#line 31 "id3_fid_reverse.gperf"
      {"LANGUAGE",			ID3FID_LANGUAGE},
#line 48 "id3_fid_reverse.gperf"
      {"NETRADIOSTATION",	ID3FID_NETRADIOSTATION},
#line 19 "id3_fid_reverse.gperf"
      {"GENRE",				ID3FID_CONTENTTYPE},
#line 23 "id3_fid_reverse.gperf"
      {"ENCODEDBY",			ID3FID_ENCODEDBY},
#line 49 "id3_fid_reverse.gperf"
      {"NETRADIOOWNER",		ID3FID_NETRADIOOWNER},
#line 32 "id3_fid_reverse.gperf"
      {"SONGLEN",			ID3FID_SONGLEN},
#line 54 "id3_fid_reverse.gperf"
      {"USERTEXT",			ID3FID_USERTEXT},
#line 56 "id3_fid_reverse.gperf"
      {"UNSYNCEDLYRICS",		ID3FID_UNSYNCEDLYRICS},
#line 38 "id3_fid_reverse.gperf"
      {"ORIGYEAR",			ID3FID_ORIGYEAR},
#line 42 "id3_fid_reverse.gperf"
      {"CONDUCTOR",			ID3FID_CONDUCTOR},
#line 35 "id3_fid_reverse.gperf"
      {"ORIGFILENAME",		ID3FID_ORIGFILENAME},
#line 47 "id3_fid_reverse.gperf"
      {"RECORDINGDATES",		ID3FID_RECORDINGDATES},
#line 16 "id3_fid_reverse.gperf"
      {"ALBUM",				ID3FID_ALBUM},
#line 37 "id3_fid_reverse.gperf"
      {"ORIGARTIST",			ID3FID_ORIGARTIST},
#line 24 "id3_fid_reverse.gperf"
      {"LYRICIST",			ID3FID_LYRICIST},
#line 55 "id3_fid_reverse.gperf"
      {"COMMENT",			ID3FID_COMMENT},
#line 26 "id3_fid_reverse.gperf"
      {"TIME",				ID3FID_TIME},
#line 28 "id3_fid_reverse.gperf"
      {"TITLE",				ID3FID_TITLE},
#line 36 "id3_fid_reverse.gperf"
      {"ORIGLYRICIST",		ID3FID_ORIGLYRICIST},
#line 45 "id3_fid_reverse.gperf"
      {"PUBLISHER",			ID3FID_PUBLISHER},
#line 34 "id3_fid_reverse.gperf"
      {"ORIGALBUM",			ID3FID_ORIGALBUM},
#line 29 "id3_fid_reverse.gperf"
      {"SUBTITLE",			ID3FID_SUBTITLE},
#line 40 "id3_fid_reverse.gperf"
      {"ARTIST",				ID3FID_LEADARTIST},
#line 22 "id3_fid_reverse.gperf"
      {"PLAYLISTDELAY",		ID3FID_PLAYLISTDELAY},
#line 51 "id3_fid_reverse.gperf"
      {"ISRC",				ID3FID_ISRC},
#line 25 "id3_fid_reverse.gperf"
      {"FILETYPE",			ID3FID_FILETYPE},
#line 33 "id3_fid_reverse.gperf"
      {"MEDIATYPE",			ID3FID_MEDIATYPE},
#line 18 "id3_fid_reverse.gperf"
      {"COMPOSER",			ID3FID_COMPOSER},
#line 39 "id3_fid_reverse.gperf"
      {"FILEOWNER",			ID3FID_FILEOWNER},
#line 46 "id3_fid_reverse.gperf"
      {"TRACKNUMBER",		ID3FID_TRACKNUM},
#line 27 "id3_fid_reverse.gperf"
      {"CONTENTGROUP",		ID3FID_CONTENTGROUP},
#line 20 "id3_fid_reverse.gperf"
      {"COPYRIGHT",			ID3FID_COPYRIGHT},
#line 66 "id3_fid_reverse.gperf"
      {"WWWUSER",			ID3FID_WWWUSER},
#line 61 "id3_fid_reverse.gperf"
      {"WWWCOMMERCIALINFO",	ID3FID_WWWCOMMERCIALINFO},
#line 60 "id3_fid_reverse.gperf"
      {"WWWAUDIOSOURCE",		ID3FID_WWWAUDIOSOURCE},
#line 43 "id3_fid_reverse.gperf"
      {"MIXARTIST",			ID3FID_MIXARTIST},
#line 30 "id3_fid_reverse.gperf"
      {"INITIALKEY",			ID3FID_INITIALKEY},
#line 57 "id3_fid_reverse.gperf"
      {"UNIQUE_FILE_ID",		ID3FID_UNIQUEFILEID},
#line 67 "id3_fid_reverse.gperf"
      {"INVOLVEDPEOPLE",		ID3FID_INVOLVEDPEOPLE},
#line 64 "id3_fid_reverse.gperf"
      {"WWWPAYMENT",			ID3FID_WWWPAYMENT},
#line 65 "id3_fid_reverse.gperf"
      {"WWWRADIOPAGE",		ID3FID_WWWRADIOPAGE},
#line 63 "id3_fid_reverse.gperf"
      {"WWWPUBLISHER",		ID3FID_WWWPUBLISHER},
#line 62 "id3_fid_reverse.gperf"
      {"WWWCOPYRIGHT",		ID3FID_WWWCOPYRIGHT},
#line 59 "id3_fid_reverse.gperf"
      {"WWWARTIST",			ID3FID_WWWARTIST},
#line 58 "id3_fid_reverse.gperf"
      {"WWWAUDIOFILE",		ID3FID_WWWAUDIOFILE},
#line 44 "id3_fid_reverse.gperf"
      {"PARTINSET",			ID3FID_PARTINSET}
    };

  static const short lookup[] =
    {
      -1, -1,  0, -1, -1, -1,  1, -1,  2, -1,  3,  4, -1, -1,
      -1,  5,  6,  7, -1, -1,  8,  9, 10, -1, -1, 11, 12, -1,
      13, -1, 14, 15, 16, -1, -1, 17, -1, 18, -1, -1, 19, 20,
      21, -1, -1, 22, 23, -1, -1, -1, 24, 25, 26, -1, -1, 27,
      28, 29, -1, -1, 30, 31, 32, -1, -1, 33, 34, 35, -1, -1,
      36, 37, -1, -1, -1, -1, 38, -1, -1, -1, 39, 40, -1, 41,
      -1, 42, 43, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, 44, -1, 45, -1, -1, 46, -1, -1, -1, -1, 47, -1,
      -1, -1, -1, 48, 49, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, 50, -1, -1, 51
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].name;

              if ((((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !gperf_case_strncmp (str, s, len) && s[len] == '\0')
                return &wordlist[index];
            }
        }
    }
  return 0;
}
#line 68 "id3_fid_reverse.gperf"


ID3_FrameID get_frame_id(const char * name)
{
	const struct id3_frame * frame = frame_lookup(name, strlen(name));
	if (frame) return frame->id;
	else return ID3FID_NOFRAME;
}
