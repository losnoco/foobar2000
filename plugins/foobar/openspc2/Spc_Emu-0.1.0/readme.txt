Spc_Emu 0.1.0: Super NES SPC Music File Emulation Library

This is a portable Super Nintendo (SNES) SPC music file emulator
(synthesizer) for use in an SPC music file player. Licensed under the
GNU General Public License (GPL); see GPL.TXT. Copyright (C) 2004 Shay
Green. DSP emulator based on OpenSPC, Copyright (C) 2002 Brad Martin.

Contact: hotpop.com@blargg (swap to e-mail)


Getting Started
---------------

This library is written in somewhat conservative C++ that should compile
with current and older compilers (ISO and ARM).

Start by building a program consisting of the included source files and
any necessary system libraries. Run it with an SPC file named "test.spc"
in the same directory and it should generate an AIFF sound file
"out.aif" of the first few seconds of the SPC.
 
See notes.txt for more information, and respective header (.h) files for
reference.


Files
-----

notes.txt           Collection of notes about the library
GPL.TXT             GNU General Public License

demo.cpp            How to use Spc_Emu to record SPC to a sound file
Sound_Writer.hpp    AIFF sound file writer used for demo output
Sound_Writer.cpp

Spc_Cpu.cpp         SPC CPU emulator
Spc_Cpu.h
Spc_Dsp.cpp         SPC DSP emulator
Spc_Dsp.h
Spc_Emu.cpp         SPC emulator environment
Spc_Emu.h
blargg_endian.h     CPU byte order utilities


-- 
Shay Green <hotpop.com@blargg> (swap to e-mail)
