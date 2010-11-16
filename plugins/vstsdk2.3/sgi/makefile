# General Makefile
#
# (c)1999 Steinberg Soft+Hardware GmbH
#

include ./makedef

all:
	smake -f adelayeditgui.mak all
	smake -f adelay.mak all
	smake -f again.mak all
	smake -f vstxsynth.mak all

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(OBJDIR)/makedepend
