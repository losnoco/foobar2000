#include <foobar2000.h>

extern cfg_int cfg_sample_rate;

extern cfg_int cfg_indefinite, cfg_default_length, cfg_default_fade;

extern cfg_int cfg_write;

extern cfg_int cfg_write_nsfe, cfg_nsfe_ignore_playlists;

extern cfg_int cfg_spc_anti_surround, cfg_spc_interpolation;

extern cfg_int cfg_vgm_gd3_prefers_japanese;

extern cfg_int cfg_control_override, cfg_control_tempo;

#define BORK_TIME 0xC0CAC01A

unsigned long parse_time_crap(const char *input);
void print_time_crap(int ms, char *out);