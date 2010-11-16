# Makefile
#
# (c)1999 Steinberg Soft+Hardware GmbH
#

include ./makedef

#define some directories
SRCDELAYEDITDIR = $(SRCDIR)/ADelayEditGUI
SRCDELAYDIR = $(SRCDIR)/ADelay

all: $(TGTDIR)/ADelayEditGUI

OBJECTS = \
	$(OBJDIR)/audioeffectx.o \
	$(OBJDIR)/AudioEffect.o \
	$(OBJDIR)/ADelayEditMain.o \
	$(OBJDIR)/ADEditor.o \
	$(OBJDIR)/ADelayEdit.o \
	$(OBJDIR)/ADelay.o


## VST class #############################################################

$(OBJDIR)/AudioEffect.o: $(VSTDIR)/AudioEffect.cpp
	$(CC) $(CCOPTS) -o $@ $(VSTDIR)/AudioEffect.cpp

$(OBJDIR)/audioeffectx.o: $(VSTDIR)/audioeffectx.cpp
	$(CC) $(CCOPTS) -o $@ $(VSTDIR)/audioeffectx.cpp


## ADelayEditGUI ###########################################################

$(OBJDIR)/ADelayEditMain.o: $(SRCDELAYEDITDIR)/ADelayEditMain.cpp
	$(CC) $(CCOPTS) -I$(SRCDELAYDIR) -o $@ $(SRCDELAYEDITDIR)/ADelayEditMain.cpp

$(OBJDIR)/ADEditor.o: $(SRCDELAYEDITDIR)/ADEditor.cpp
	$(CC) $(CCOPTS) -I$(SRCDELAYDIR) -o $@ $(SRCDELAYEDITDIR)/ADEditor.cpp

$(OBJDIR)/ADelayEdit.o: $(SRCDELAYEDITDIR)/ADelayEdit.cpp
	$(CC) $(CCOPTS) -I$(SRCDELAYDIR) -o $@ $(SRCDELAYEDITDIR)/ADelayEdit.cpp

$(OBJDIR)/ADelay.o: $(SRCDELAYDIR)/ADelay.cpp
	$(CC) $(CCOPTS) -o $@ $(SRCDELAYDIR)/ADelay.cpp

$(TGTDIR)/ADelayEditGUI: $(OBJECTS)
	$(CC) $(LOPTS) -o $(TGTDIR)/ADelayEditGUI $(OBJECTS) $(LIBS) $(GUILIBS)
