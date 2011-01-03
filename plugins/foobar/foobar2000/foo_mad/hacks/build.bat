@echo off

set C_OPT=-fomit-frame-pointer -funroll-loops -ffunction-sections -fdata-sections

if _%1_==_profile_ goto profile

gcc -O3 %C_OPT% -fbranch-probabilities -I . -D NDEBUG -D WIN32 -D_MBCS -D _LIB -D HAVE_CONFIG_H -D FPM_INTEL -D ASO_ZEROCHECK -c bit.c decoder.c fixed.c frame.c huffman.c layer12.c layer3.c stream.c synth.c timer.c version.c

goto end

:profile

del *.da
gcc -O3 %C_OPT -fprofile-arcs -I . -D NDEBUG -D WIN32 -D_MBCS -D _LIB -D HAVE_CONFIG_H -D FPM_INTEL -D ASO_ZEROCHECK bit.c decoder.c fixed.c frame.c huffman.c layer12.c layer3.c stream.c synth.c timer.c version.c minimad.c -o minimad
strip minimad.exe

:end

set C_OPT=
