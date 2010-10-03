TIMESTAMP = `date +%Y%m%d`
ZIPFILE32 = /tmp/proxy-libintl-dev_$(TIMESTAMP)_win32.zip
ZIPFILE64 = /tmp/proxy-libintl-dev_$(TIMESTAMP)_win64.zip

CC = gcc
CPPFLAGS = -I ../../include
CFLAGS = -Wall $(CPPFLAGS)

X64CC = x86_64-pc-mingw32-gcc

X64MSVC = cl

all : \
	../../lib/libintl.a 		\
	../../lib/intl.lib		\
	../../win64/include/libintl.h	\
	../../win64/lib/libintl.a	\
	../../win64/lib/intl.lib

../../lib/libintl.a : libintl.o
	ar rc $@ libintl.o

../../lib/intl.lib : ../../lib/libintl.a
	cp ../../lib/libintl.a $@
	strip --strip-unneeded $@

../../win64/include/libintl.h : ../../include/libintl.h
	mkdir -p `dirname $@`
	cp -p ../../include/libintl.h $@

../../win64/lib/libintl.a : libintl.x64.o
	mkdir -p `dirname $@`
	x86_64-pc-mingw32-ar rc $@ libintl.x64.o

../../win64/lib/intl.lib : libintl.vs.x64.o
	mkdir -p `dirname $@`
	lib -machine:x64 -out:$@ libintl.vs.x64.o

libintl.o : libintl.c
	$(CC) $(CFLAGS) -c libintl.c

libintl.x64.o : libintl.c
	$(X64CC) $(CFLAGS) -c -o $@ libintl.c

libintl.vs.x64.o : libintl.c
	$(X64MSVC) -MD -Zi $(CPPFLAGS) -c -Fo$@ libintl.c

clean :
	rm -rf *.o ../../lib/* ../../win64 $(ZIPFILE32) $(ZIPFILE64)

dist : clean all
	rm -f $(ZIPFILE32) $(ZIPFILE64)
	cd ../..; zip $(ZIPFILE32) include/libintl.h lib/{libintl.a,intl.lib} src/proxy-libintl/{README.txt,COPYING.LIB.txt,Makefile,libintl.c}; manifestify $(ZIPFILE)
	cd ../../win64; zip $(ZIPFILE64) include/libintl.h
	cd ../../win64; zip $(ZIPFILE64) lib/{libintl.a,intl.lib}
	cd ../..; zip $(ZIPFILE64) src/proxy-libintl/{README.txt,COPYING.LIB.txt,Makefile,libintl.c};
	manifestify $(ZIPFILE32) $(ZIPFILE64)
