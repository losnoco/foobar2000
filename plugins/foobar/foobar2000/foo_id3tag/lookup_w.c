/* C code produced by gperf version 3.0.1 */
/* Command-line: gperf -tCcTonD -K long_name -N write_lookup --ignore-case lookup.gperf  */
/* Computed positions: -k'1-4' */

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

#line 1 "lookup.gperf"


typedef void (* t_process_callback)(const char *, const char *);

typedef bool (* t_validate)(struct id3_frame *);
typedef void (* t_process)(struct id3_frame *, t_callback);
typedef void (* t_render)(struct id3_frame *, int, const char **);

#define DECLAREFUNCS(type) extern t_validate type##_validate; extern t_process type##_process; extern t_render type##_render;
#define DECLAREFUNCSNV(type) extern t_process type##_process; extern t_render type##_render;

DECLAREFUNCSNV(text)
DECLAREFUNCS(url)
DECLAREFUNCS(urlx)
DECLAREFUNCS(comment)
DECLAREFUNCS(uniqueid)
DECLAREFUNCS(unsyncedlyrics)

typedef struct t_id3_lookup {
    const char * short;
    const char * long;
    t_validate validate;
    t_process process;
    t_render render;
};

#define FV(t)    t##_validate, t##_process, t##_render
#define FN(t)               0, t##_process, t##_render


#define TOTAL_KEYWORDS 58
#define MIN_WORD_LENGTH 4
#define MAX_WORD_LENGTH 4
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 176
/* maximum key range = 175, duplicates = 0 */

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
/*ARGSUSED*/
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned char asso_values[] =
    {
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
       31,  26,  60,  35, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177,  25,  26,  25,   1,  60,
       11,   5,   5,  30, 177,   5,  25,  55,  50,   5,
       10,  12,   0,   0,   1,  25, 177,  35,  15,  51,
       41, 177, 177, 177, 177, 177, 177,  25,  26,  25,
        1,  60,  11,   5,   5,  30, 177,   5,  25,  55,
       50,   5,  10,  12,   0,   0,   1,  25, 177,  35,
       15,  51,  41, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177, 177, 177, 177,
      177, 177, 177, 177, 177, 177, 177
    };
  return asso_values[(unsigned char)str[3]+1] + asso_values[(unsigned char)str[2]] + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#endif
const struct t_id3_lookup *
write_lookup (str, len)
     register const char *str;
     register unsigned int len;
{
  static const struct t_id3_lookup wordlist[] =
    {
#line 64 "lookup.gperf"
      {"TSRC", "isrc", FN(text)},
#line 80 "lookup.gperf"
      {"TDRC", "recording_time", FN(text)},
#line 62 "lookup.gperf"
      {"TRSN", "internet_radio_station_name", FN(text)},
#line 79 "lookup.gperf"
      {"TDOR", "original_release_time", FN(text)},
#line 82 "lookup.gperf"
      {"TDTG", "tagging_time", FN(text)},
#line 63 "lookup.gperf"
      {"TRSO", "internet_radio_station_owner", FN(text)},
#line 65 "lookup.gperf"
      {"TSSE", "encoder_settings", FN(text)},
#line 59 "lookup.gperf"
      {"TPOS", "part_of_set", FN(text)},
#line 88 "lookup.gperf"
      {"TSOP", "performer_sort_order", FN(text)},
#line 86 "lookup.gperf"
      {"TPRO", "produced_notice", FN(text)},
#line 51 "lookup.gperf"
      {"TOFN", "original_filename", FN(text)},
#line 90 "lookup.gperf"
      {"TSST", "set_subtitle", FN(text)},
#line 53 "lookup.gperf"
      {"TOPE", "original_artist", FN(text)},
#line 89 "lookup.gperf"
      {"TSOT", "title_sort_order", FN(text)},
#line 87 "lookup.gperf"
      {"TSOA", "album_sort_order", FN(text)},
#line 37 "lookup.gperf"
      {"TCON", "genre", FN(text)},
#line 74 "lookup.gperf"
      {"WORS", "url_station", FV(url)},
#line 38 "lookup.gperf"
      {"TCOP", "copyright", FN(text)},
#line 54 "lookup.gperf"
      {"TOWN", "file_owner", FN(text)},
#line 61 "lookup.gperf"
      {"TRCK", "tracknumber", FN(text)},
#line 47 "lookup.gperf"
      {"TLAN", "language", FN(text)},
#line 81 "lookup.gperf"
      {"TDRL", "release_time", FN(text)},
#line 44 "lookup.gperf"
      {"TIT2", "title", FN(text)},
#line 60 "lookup.gperf"
      {"TPUB", "publisher", FN(text)},
#line 42 "lookup.gperf"
      {"TFLT", "file_type", FN(text)},
#line 43 "lookup.gperf"
      {"TIT1", "content_group_description", FN(text)},
#line 72 "lookup.gperf"
      {"WOAR", "url_artist", FV(url)},
#line 73 "lookup.gperf"
      {"WOAS", "url_source", FV(url)},
#line 78 "lookup.gperf"
      {"TDEN", "encoding_time", FN(text)},
#line 39 "lookup.gperf"
      {"TDLY", "playlist_delay", FN(text)},
#line 71 "lookup.gperf"
      {"WOAF", "url_file", FV(url)},
#line 85 "lookup.gperf"
      {"TMOO", "mood", FN(text)},
#line 52 "lookup.gperf"
      {"TOLY", "original_lyricist", FN(text)},
#line 68 "lookup.gperf"
      {"USLT", "unsynchronised_lyrics", FV(unsyncedlyrics)},
#line 34 "lookup.gperf"
      {"TALB", "album", FN(text)},
#line 70 "lookup.gperf"
      {"WCOP", "copyright_information", FV(url)},
#line 36 "lookup.gperf"
      {"TCOM", "composer", FN(text)},
#line 66 "lookup.gperf"
      {"TXXX", "user_text", FN(text)},
#line 50 "lookup.gperf"
      {"TOAL", "original_album_title", FN(text)},
#line 35 "lookup.gperf"
      {"TBPM", "beats_per_minute", FN(text)},
#line 48 "lookup.gperf"
      {"TLEN", "length", FN(text)},
#line 45 "lookup.gperf"
      {"TIT3", "subtitle", FN(text)},
#line 76 "lookup.gperf"
      {"WPUB", "url_publisher", FV(url)},
#line 83 "lookup.gperf"
      {"TIPL", "involved_people_list", FN(text)},
#line 56 "lookup.gperf"
      {"TPE2", "band", FN(text)},
#line 41 "lookup.gperf"
      {"TEXT", "lyricist", FN(text)},
#line 55 "lookup.gperf"
      {"TPE1", "artist", FN(text)},
#line 58 "lookup.gperf"
      {"TPE4", "remixer", FN(text)},
#line 46 "lookup.gperf"
      {"TKEY", "initial_key", FN(text)},
#line 75 "lookup.gperf"
      {"WPAY", "payment", FV(url)},
#line 40 "lookup.gperf"
      {"TENC", "encoded_by", FN(text)},
#line 69 "lookup.gperf"
      {"WCOM", "commercial", FV(url)},
#line 77 "lookup.gperf"
      {"WXXX", "url_user", FV(urlx)},
#line 67 "lookup.gperf"
      {"UFID", "unique_file_identifier", FV(uniqueid)},
#line 57 "lookup.gperf"
      {"TPE3", "conductor", FN(text)},
#line 33 "lookup.gperf"
      {"COMM", "comment", FV(comment)},
#line 84 "lookup.gperf"
      {"TMCL", "musician_credits_list", FN(text)},
#line 49 "lookup.gperf"
      {"TMED", "media_type", FN(text)}
    };

  static const short lookup[] =
    {
      -1, -1,  0,  1, -1, -1,  2,  3,  4, -1, -1,  5,  6, -1,
      -1, -1, -1,  7,  8, -1, -1,  9, 10, -1, -1, -1, 11, 12,
      -1, -1, -1, 13, 14, -1, -1, -1, 15, -1, -1, -1, -1, 16,
      -1, 17, -1, -1, 18, -1, -1, -1, -1, 19, -1, -1, -1, -1,
      20, 21, 22, -1, -1, 23, 24, 25, -1, 26, 27, 28, 29, -1,
      30, 31, 32, -1, -1, 33, 34, 35, -1, -1, -1, 36, 37, -1,
      -1, -1, 38, 39, -1, -1, -1, 40, 41, -1, -1, 42, 43, 44,
      -1, -1, -1, 45, 46, -1, -1, -1, 47, 48, -1, -1, -1, 49,
      50, -1, -1, 51, 52, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      53, -1, -1, -1, -1, 54, -1, -1, -1, 55, 56, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, 57
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].long_name;

              if ((((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !gperf_case_strncmp (str, s, len) && s[len] == '\0')
                return &wordlist[index];
            }
        }
    }
  return 0;
}
