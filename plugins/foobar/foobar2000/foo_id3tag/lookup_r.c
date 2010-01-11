/* C code produced by gperf version 3.0.1 */
/* Command-line: gperf -tCcTonD -K short_name -N read_lookup -s -3 -k '*'  */

#if 0
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
#endif



#include "lookup.h"

#include <string.h>

#define RV(t) t##_validate, t##_process
#define RN(t)            0, t##_process

#define TOTAL_KEYWORDS 57
#define MIN_WORD_LENGTH 4
#define MAX_WORD_LENGTH 4
#define MIN_HASH_VALUE 19
#define MAX_HASH_VALUE 101
/* maximum key range = 83, duplicates = 0 */

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
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
       16,   6,  30,   3, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102,   1,  11,   3,  15,  21,
        2,   5,  10,  15, 102,  15,  25,  30,  18,   5,
       10,   1,  20,   0,  28,   5, 102,  13,  20,  31,
       28, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102
    };
  return asso_values[(unsigned char)str[3]+1] + asso_values[(unsigned char)str[2]] + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#endif
const struct t_read_lookup *
read_lookup (str, len)
     register const char *str;
     register unsigned int len;
{
  static const struct t_read_lookup wordlist[] =
    {
      {"WOAR", "url_artist", RV(url)},
      {"WCOP", "copyright_information", RV(url)},
      {"WOAF", "url_file", RV(url)},
      {"TSSE", "encoder_settings", RV(text)},
      {"WPUB", "url_publisher", RV(url)},
      {"TSST", "set_subtitle", RV(text)},
      {"TSOP", "performer_sort_order", RV(text)},
      {"TCOP", "copyright", RV(text)},
      {"TSOT", "title_sort_order", RV(text)},
      {"WCOM", "commercial", RV(url)},
      {"TOFN", "original_filename", RV(text)},
      {"TCON", "genre", RV(text)},
      {"UFID", "unique_file_identifier", RV(uniqueid)},
      {"TSOA", "album_sort_order", RV(text)},
      {"TOPE", "original_artist", RV(text)},
      {"TPUB", "publisher", RV(text)},
      {"WOAS", "url_source", RV(url)},
      {"TDOR", "original_release_time", RV(text)},
      {"TOWN", "file_owner", RV(text)},
      {"WPAY", "payment", RV(url)},
      {"TRSN", "internet_radio_station_name", RV(text)},
      {"TCOM", "composer", RV(text)},
      {"COMM", "comment", RV(comment)},
      {"TALB", "album", RV(text)},
      {"TRSO", "internet_radio_station_owner", RV(text)},
      {"TLAN", "language", RV(text)},
      {"TFLT", "file_type", RV(text)},
      {"TPE4", "remixer", RV(text)},
      {"TSRC", "isrc", RV(text)},
      {"TOAL", "original_album_title", RV(text)},
      {"TPE2", "band", RV(text)},
      {"WORS", "url_station", RV(url)},
      {"TBPM", "beats_per_minute", RV(text)},
      {"TPRO", "produced_notice", RV(text)},
      {"TDEN", "encoding_time", RV(text)},
      {"TPOS", "part_of_set", RV(text)},
      {"TMOO", "mood", RV(text)},
      {"TEXT", "lyricist", RV(text)},
      {"TPE1", "artist", RV(text)},
      {"TRCK", "tracknumber", RV(text)},
      {"TIT2", "title", RV(text)},
      {"TDRC", "recording_time", RV(text)},
      {"TLEN", "length", RV(text)},
      {"TDTG", "tagging_time", RV(text)},
      {"TENC", "encoded_by", RV(text)},
      {"TIPL", "involved_people_list", RV(text)},
      {"WXXX", "url_user", RV(urlx)},
      {"TOLY", "original_lyricist", RV(text)},
      {"TIT1", "content_group_description", RV(text)},
      {"TPE3", "conductor", RV(text)},
      {"TMCL", "musician_credits_list", RV(text)},
      {"TKEY", "initial_key", RV(text)},
      {"TDRL", "release_time", RV(text)},
      {"TDLY", "playlist_delay", RV(text)},
      {"TXXX", "user_text", RV(textx)},
      {"TMED", "media_type", RV(text)},
      {"TIT3", "subtitle", RV(text)}
    };

  static const signed char lookup[] =
    {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1,  0, -1, -1,  1, -1,  2, -1, -1, -1,
      -1, -1,  3,  4, -1,  5,  6, -1, -1,  7,  8,  9, 10, 11,
      -1, 12, 13, 14, 15, 16, 17, -1, -1, 18, 19, 20, 21, -1,
      22, 23, 24, 25, 26, -1, 27, 28, 29, 30, 31, 32, 33, 34,
      -1, 35, -1, 36, 37, 38, 39, 40, 41, 42, -1, 43, 44, 45,
      46, -1, 47, 48, -1, 49, -1, 50, 51, 52, -1, -1, 53, -1,
      -1, 54, 55, 56
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].short_name;

              if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                return &wordlist[index];
            }
        }
    }
  return 0;
}
