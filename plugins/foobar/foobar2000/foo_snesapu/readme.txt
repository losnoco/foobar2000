This is a wrapper for the SNESAPU.DLL SPC decoding library for foobar2000, built with fb2k's SDK version 0.8.2.

To build this from source, you will need to:

- extract the foo_snesapu folder to your *\0.8.2-SDK\foobar2000 directory
- add the following files to the foo_snesapu directory from SNESAPU's source tree:
  - APU.h
  - DSP.h
  - SNESAPU.h
  - SPC700.h
  - Types.h 
- build as per usual

You can get SNESAPU 2.0's source code from http://www.alpha-ii.com/Source/index.html

To actually use the component, SNESAPU.DLL must be somewhere in your path, be it in %WINDIR% or in the same directory as foobar2000.exe.  I couldn't find a recent build of SNESAPU.DLL on the web, so it should be built from the source distribution (requires nasm to build).

Enjoy!