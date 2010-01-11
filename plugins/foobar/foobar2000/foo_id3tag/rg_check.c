/* C code produced by gperf version 3.0.1 */
/* Command-line: gperf -CcTonD -K name -N rg_check -s -3 -k '*' rg_check.gperf  */

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

#line 1 "rg_check.gperf"

#include "rg_check.h"

#include <string.h>


#define TOTAL_KEYWORDS 4
#define MIN_WORD_LENGTH 21
#define MAX_WORD_LENGTH 21
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 5
/* maximum key range = 4, duplicates = 0 */

__inline
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned char asso_values[] =
    {
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 0, 6, 0, 1, 0,
      6, 1, 6, 0, 6, 0, 6, 0, 1, 0,
      0, 6, 0, 6, 0, 6, 0, 0, 6, 6,
      6, 0, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6
    };
  return asso_values[(unsigned char)str[20]] + asso_values[(unsigned char)str[19]] + asso_values[(unsigned char)str[18]] + asso_values[(unsigned char)str[17]] + asso_values[(unsigned char)str[16]] + asso_values[(unsigned char)str[15]] + asso_values[(unsigned char)str[14]] + asso_values[(unsigned char)str[13]] + asso_values[(unsigned char)str[12]] + asso_values[(unsigned char)str[11]] + asso_values[(unsigned char)str[10]] + asso_values[(unsigned char)str[9]] + asso_values[(unsigned char)str[8]] + asso_values[(unsigned char)str[7]] + asso_values[(unsigned char)str[6]] + asso_values[(unsigned char)str[5]] + asso_values[(unsigned char)str[4]] + asso_values[(unsigned char)str[3]] + asso_values[(unsigned char)str[2]] + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]];
}

__inline
static const char *
rg_check (str, len)
     register const char *str;
     register unsigned int len;
{
  static const char * const wordlist[] =
    {
      "replaygain_track_gain",
      "replaygain_track_peak",
      "replaygain_album_gain",
      "replaygain_album_peak"
    };

  static const signed char lookup[] =
    {
      -1, -1,  0,  1,  2,  3
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index];

              if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                return s;
            }
        }
    }
  return 0;
}
#line 11 "rg_check.gperf"


int is_replaygain(const char * name)
{
	const char * s = rg_check(name, strlen(name));
	if (s) return 1;
	else return 0;
}
