# Makefile
#
# (c)1999 Steinberg Soft+Hardware GmbH
#

include ./makedef

#define some directories
SRCGAINDIR = $(SRCDIR)/AGain

include makedef

all: $(TGTDIR)/AGain

OBJECTS = \
	$(OBJDIR)/audioeffect.o \
	$(OBJDIR)/audioeffectx.o \
	$(OBJDIR)/AGainMain.o \
	$(OBJDIR)/AGain.o


## Base class #############################################################

$(OBJDIR)/audioeffectx.o: $(VSTDIR)/audioeffectx.cpp
	$(CC) $(CCOPTS) -o $@ $(VSTDIR)/audioeffectx.cpp

$(OBJDIR)/audioeffect.o: $(VSTDIR)/AudioEffect.cpp
	$(CC) $(CCOPTS) -o $@ $(VSTDIR)/AudioEffect.cpp


## Moog ###########################################################

$(OBJDIR)/AGainMain.o: $(SRCGAINDIR)/AGainMain.cpp
	$(CC) $(CCOPTS) -o $@ $(SRCGAINDIR)/AGainMain.cpp

$(OBJDIR)/AGain.o: $(SRCGAINDIR)/AGain.cpp
	$(CC) $(CCOPTS) -o $@ $(SRCGAINDIR)/AGain.cpp

$(TGTDIR)/AGain: $(OBJECTS)
	$(CC) $(LOPTS) -o $@ $(OBJECTS) $(LIBS)
