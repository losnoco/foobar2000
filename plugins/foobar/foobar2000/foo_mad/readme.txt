You will to apply the included patch to libmad-0.15.1b for it to work properly.

Also, until the player SDK is updated, you will need the included mpeg_decoder.h
file. You can either alter mad_interface.cpp to point to it as-is, or mimic how
things will probably be with the next SDK. See comments in mpeg_decoder.h.


Oh yes, if you want to build a libmad binary as fast as mine, you will also need
to use this dirty collection of hacks which will compile it with the latest
MinGW and manipulate the objects so they will link properly with MSVC.

objfix2 is derived from objfix.c, by TRAC, which was designed for converting
from MSVC objects to MinGW.

This software, excluding mpeg_decoder.h and objfix2, is licensed under the GNU
General Public License. See COPYING.TXT.

mpeg_decoder.h is (or will be) available as part of the Foobar2000 SDK, which is
available under the BSD license. Yadda yadda...

objfix2 ... I dunno. Maybe you should drop into #zsnes on Freenode and ask TRAC. :B
