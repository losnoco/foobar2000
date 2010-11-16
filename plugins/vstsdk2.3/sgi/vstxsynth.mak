# Makefile
#
# (c)1999 Steinberg Soft+Hardware GmbH
#

include ./makedef

#define some directories
SRCDIR2 = $(SRCDIR)/vstxsynth

include makedef

all: $(TGTDIR)/Vstxsynth

OBJECTS = \
	$(OBJDIR)/audioeffect.o \
	$(OBJDIR)/audioeffectx.o \
	$(OBJDIR)/vstxsynthmain.o \
	$(OBJDIR)/vstxsynth.o \
	$(OBJDIR)/vstxsynthproc.o

## Base class #############################################################

$(OBJDIR)/audioeffectx.o: $(VSTDIR)/audioeffectx.cpp
	$(CC) $(CCOPTS) -o $@ $(VSTDIR)/audioeffectx.cpp

$(OBJDIR)/audioeffect.o: $(VSTDIR)/AudioEffect.cpp
	$(CC) $(CCOPTS) -o $@ $(VSTDIR)/AudioEffect.cpp


## Moog ###########################################################

$(OBJDIR)/vstxsynthmain.o: $(SRCDIR2)/vstxsynthmain.cpp
	$(CC) $(CCOPTS) -o $@ $(SRCDIR2)/vstxsynthmain.cpp

$(OBJDIR)/vstxsynth.o: $(SRCDIR2)/vstxsynth.cpp
	$(CC) $(CCOPTS) -o $@ $(SRCDIR2)/vstxsynth.cpp

$(OBJDIR)/vstxsynthproc.o: $(SRCDIR2)/vstxsynthproc.cpp
	$(CC) $(CCOPTS) -o $@ $(SRCDIR2)/vstxsynthproc.cpp

$(TGTDIR)/Vstxsynth: $(OBJECTS)
	$(CC) $(LOPTS) -o $@ $(OBJECTS) $(LIBS)
