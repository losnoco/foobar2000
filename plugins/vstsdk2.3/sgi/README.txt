#-------------------------------------------------------------------------
# README  -  VST Plug-Ins SDK
# version 2.0 extension
# (c)1999 Steinberg Soft+Hardware GmbH
#-------------------------------------------------------------------------


  #-------------------------------------------------------------------------
  1. On-line documentations are available in the following location:

  vstsdk2/VST Plug-Ins SDK 2.0.pdf
  vstsdk2/doc/gui/index.html (for VSTGUI)


  #-------------------------------------------------------------------------
  2. This directory <sgi> contains:
  
  <README>   : this file.

  <resource> : directory which contains the bitmaps of the plugin.
               The format of these bitmaps is XPM. They are included during
               the compilation as a included file.

  <debug>    : directory which will contain the object files
               for the debug mode.

  <release>  : directory which will contain the object files 
               for the release mode.

  <library>  : directory which contains the static VST-GUI library: libvstgui.a

  <sgidef>   : included file in makedef file which specifies some global
               settings (platform dependent).

  <makedef>  : file which defines settings for the compilation.
               (It is included in the makefile).

  <makefile> : global makefile.

  <adelayeditgui.mak> : makefile for the ADelayEditGui plugin (with GUI).
  <adelay.mak>        : makefile for the ADelay plugin.
  <again.mak>         : makefile for the AGain plugin.
  <vstxsynth.mak>     : makefile for the Vstxsynth plugin.


  #-------------------------------------------------------------------------
  3. To compile this AdelayEditGUI example, type in a winterm 
     (in the directory vstsdk2/sgi):

  % smake -f adelayeditgui.mak all

  Note: This makefile project has been tested on a SGI IRIX 6.3 with the SGI
        C++ compiler. It uses the smake command.
        The output file (dynamic library) is created in this sgi directory.

  #-------------------------------------------------------------------------
  4. If you want to build a release version of this plugin,
     just set the environment variable SHIPPING to 1 with:

  % setenv SHIPPING 1

  #-------------------------------------------------------------------------
  5. If you want to unset the SHIPPING variable, then use:

  %unsetenv SHIPPING


