# Makefile
#
# (c)1999 Steinberg Soft+Hardware GmbH
#

include ./makedef

#define some directories
SRCDELAYDIR = $(SRCDIR)/ADelay

include makedef

all: $(TGTDIR)/ADelay

OBJECTS = \
	$(OBJDIR)/audioeffect.o \
	$(OBJDIR)/audioeffectx.o \
	$(OBJDIR)/ADelayMain.o \
	$(OBJDIR)/ADelay.o


## Base class #############################################################

$(OBJDIR)/audioeffectx.o: $(VSTDIR)/audioeffectx.cpp
	$(CC) $(CCOPTS) -o $@ $(VSTDIR)/audioeffectx.cpp

$(OBJDIR)/audioeffect.o: $(VSTDIR)/AudioEffect.cpp
	$(CC) $(CCOPTS) -o $@ $(VSTDIR)/AudioEffect.cpp


## Moog ###########################################################

$(OBJDIR)/ADelayMain.o: $(SRCDELAYDIR)/ADelayMain.cpp
	$(CC) $(CCOPTS) -o $@ $(SRCDELAYDIR)/ADelayMain.cpp

$(OBJDIR)/ADelay.o: $(SRCDELAYDIR)/ADelay.cpp
	$(CC) $(CCOPTS) -o $@ $(SRCDELAYDIR)/ADelay.cpp

$(TGTDIR)/ADelay: $(OBJECTS)
	$(CC) $(LOPTS) -o $@ $(OBJECTS) $(LIBS)
