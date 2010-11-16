# Microsoft Developer Studio Project File - Name="delayeditgui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=delayeditgui - Win32 Debug with VSTGUI source
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "delayeditgui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "delayeditgui.mak" CFG="delayeditgui - Win32 Debug with VSTGUI source"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "delayeditgui - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "delayeditgui - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "delayeditgui - Win32 Debug with VSTGUI source" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "delayeditgui - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /MT /W3 /GX /O2 /I "../source/common" /I "../source/ADelay" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D WINDOWS=1 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib msimg32.lib msvcrt.lib Oldnames.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib /out:"VstPlugins/DelayEditGUI.dll" /libpath:"library"

!ELSEIF  "$(CFG)" == "delayeditgui - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /GX /ZI /Od /I "../source/common" /I "../source/ADelay" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D WINDOWS=1 /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib msimg32.lib msvcrt.lib Oldnames.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /out:"VstPlugins/DelayEditGUI.dll" /pdbtype:sept /libpath:"library"

!ELSEIF  "$(CFG)" == "delayeditgui - Win32 Debug with VSTGUI source"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "delayeditgui___Win32_Debug_with_VSTGUI_source"
# PROP BASE Intermediate_Dir "delayeditgui___Win32_Debug_with_VSTGUI_source"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "debug"
# PROP Intermediate_Dir "debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /MTd /W3 /Gm /GX /Zi /Od /I "../source/common" /I "../source/ADelay" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D WINDOWS=1 /YX /FD /GZ /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /GX /Zi /Od /I "../source/common" /I "../source/ADelay" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D WINDOWS=1 /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib vstgui.lib libc.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:"library"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib msimg32.lib msvcrt.lib Oldnames.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:"library"

!ENDIF 

# Begin Target

# Name "delayeditgui - Win32 Release"
# Name "delayeditgui - Win32 Debug"
# Name "delayeditgui - Win32 Debug with VSTGUI source"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\source\ADelayEditGUI\ADEditor.cpp
# End Source File
# Begin Source File

SOURCE=..\source\ADelayEditGUI\ADEditor.hpp
# End Source File
# Begin Source File

SOURCE=..\source\ADelay\ADelay.cpp
# End Source File
# Begin Source File

SOURCE=..\source\ADelay\ADelay.hpp
# End Source File
# Begin Source File

SOURCE=..\source\ADelayEditGUI\ADelayEdit.cpp
# End Source File
# Begin Source File

SOURCE=..\source\ADelayEditGUI\ADelayEdit.hpp
# End Source File
# Begin Source File

SOURCE=..\source\ADelayEditGUI\ADelayEditMain.cpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\resource\delayeditgui\bmp00128.bmp
# End Source File
# Begin Source File

SOURCE=..\resource\delayeditgui\bmp00129.bmp
# End Source File
# Begin Source File

SOURCE=..\resource\delayeditgui\bmp00130.bmp
# End Source File
# Begin Source File

SOURCE=..\resource\delayeditgui\DelayEditGUI.rc
# End Source File
# End Group
# Begin Group "VSTGUI Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\source\Common\aeffguieditor.cpp

!IF  "$(CFG)" == "delayeditgui - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "delayeditgui - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "delayeditgui - Win32 Debug with VSTGUI source"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\source\Common\aeffguieditor.h
# End Source File
# Begin Source File

SOURCE=..\source\Common\vstcontrols.cpp

!IF  "$(CFG)" == "delayeditgui - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "delayeditgui - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "delayeditgui - Win32 Debug with VSTGUI source"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\source\Common\vstcontrols.h
# End Source File
# Begin Source File

SOURCE=..\source\Common\vstgui.cpp

!IF  "$(CFG)" == "delayeditgui - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "delayeditgui - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "delayeditgui - Win32 Debug with VSTGUI source"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\source\Common\vstgui.h
# End Source File
# Begin Source File

SOURCE=.\library\vstgui.lib

!IF  "$(CFG)" == "delayeditgui - Win32 Release"

!ELSEIF  "$(CFG)" == "delayeditgui - Win32 Debug"

!ELSEIF  "$(CFG)" == "delayeditgui - Win32 Debug with VSTGUI source"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "VST Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\source\common\AEffect.h
# End Source File
# Begin Source File

SOURCE=..\source\common\aeffectx.h
# End Source File
# Begin Source File

SOURCE=..\source\common\AEffEditor.hpp
# End Source File
# Begin Source File

SOURCE=..\source\common\AudioEffect.cpp
# End Source File
# Begin Source File

SOURCE=..\source\common\AudioEffect.hpp
# End Source File
# Begin Source File

SOURCE=..\source\common\audioeffectx.cpp
# End Source File
# Begin Source File

SOURCE=..\source\common\audioeffectx.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\delayeditgui.def
# End Source File
# End Target
# End Project
