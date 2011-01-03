Packed_Spc 0.1.0: SNES SPC Music File Compression Library

This is a portable Super Nintendo (SNES) SPC music file compression
library to allow common data for a set of SPC tracks to be stored
separately in a shared data file, to reduce the size on disk. This
preserves the file layout while allowing compression similar to RAR.
Licensed under the GNU Lesser General Public License (LGPL); see
lgpl.txt. Copyright (C) 2004 Shay Green.

Contact: hotpop.com@blargg (swap to e-mail)

Notes
-----

The documentation isn't very polished, nor has the library's interface
been reviewed by others for completeness.

I wrote simple packer and unpacker tools as an example of usage. There
are a few packed SPC files included as a test (from Donkey Kong
Country). I don't have POSIX headers otherwise I'd make them
automatically scan the directory. They can be unpacked and repacked with
the following command lines (assuming I have the UNIX relative paths
correct):

    unpack_spc_files /packed/ /unpacked/ cave.spc jungle.spc water.spc

    pack_spc_files /unpacked/ /packed/ cave.spc jungle.spc water.spc

The packed SPC files are kept gzipped. The format after ungzipping has
the same structure as a normal SPC with the packed data in place of the
usual 64KB RAM (with appropriate padding). The example packer store the
shared data in "shared.dat", though a proper tool would name the shared
file after the game to allow multiple games in the same directory. A
packed SPC file stores a copy of the name of the shared file, allowing
the packer tool to choose any name.


Files
-----

lgpl.txt                GNU Lesser General Public License

pack_spc_files.cpp      Example command-line tool to pack SPC files
unpack_spc_files.cpp    Example command-line tool to unpack SPC files

unpack_spc.c            SPC unpacker
unpack_spc.h
Spc_Packer.cpp          SPC packer
Spc_Packer.h


-- 
Shay Green <hotpop.com@blargg>


