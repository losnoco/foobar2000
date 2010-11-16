# Microsoft Developer Studio Project File - Name="SurroundDelay" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SurroundDelay - Win32 Debug with VSTGUI Source
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "surrounddelay.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "surrounddelay.mak" CFG="SurroundDelay - Win32 Debug with VSTGUI Source"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SurroundDelay - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SurroundDelay - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SurroundDelay - Win32 Debug with VSTGUI Source" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SurroundDelay - Win32 Release"

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
# ADD CPP /nologo /G6 /MT /W3 /GX /O2 /Oy- /I "..\include" /I "..\source\common" /I "..\source\adelay" /I "..\source\ADelayEditGUI" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib msimg32.lib msvcrt.lib Oldnames.lib /nologo /subsystem:windows /dll /map /machine:I386 /nodefaultlib /out:"VstPlugins/SurroundDelay.dll"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "SurroundDelay - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "surrounddelay_"
# PROP BASE Intermediate_Dir "surrounddelay_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /Gi /GX /ZI /Od /I "..\include" /I "..\source\common" /I "..\source\adelay" /I "..\source\ADelayEditGUI" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib msimg32.lib msvcrt.lib Oldnames.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /out:"VstPlugins/SurroundDelay.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "SurroundDelay - Win32 Debug with VSTGUI Source"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "SurroundDelay___Win32_Debug_with_VSTGUI_Source"
# PROP BASE Intermediate_Dir "SurroundDelay___Win32_Debug_with_VSTGUI_Source"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /MTd /W3 /Gm /Gi /GX /ZI /Od /I "..\include" /I "..\source\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /Gi /GX /ZI /Od /I "..\include" /I "..\source\common" /I "..\source\adelay" /I "..\source\ADelayEditGUI" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"VstPlugins/surrounddelay.dll" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib msimg32.lib msvcrt.lib Oldnames.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /out:"C:\Programme\Steinberg\VSTPlugins\SurroundDelay.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "SurroundDelay - Win32 Release"
# Name "SurroundDelay - Win32 Debug"
# Name "SurroundDelay - Win32 Debug with VSTGUI Source"
# Begin Group "Source Files"

# PROP Default_Filter ""
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

SOURCE=..\source\surrounddelay\surrounddelay.cpp
# End Source File
# Begin Source File

SOURCE=..\source\surrounddelay\surrounddelay.hpp
# End Source File
# Begin Source File

SOURCE=..\source\surrounddelay\surrounddelaymain.cpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ".rc"
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

SOURCE=..\resource\delayeditgui\surrounddelay.rc
# End Source File
# End Group
# Begin Group "VSTGUI Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\source\common\aeffguieditor.cpp

!IF  "$(CFG)" == "SurroundDelay - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SurroundDelay - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SurroundDelay - Win32 Debug with VSTGUI Source"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\source\Common\aeffguieditor.h
# End Source File
# Begin Source File

SOURCE=..\source\common\vstcontrols.cpp

!IF  "$(CFG)" == "SurroundDelay - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SurroundDelay - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SurroundDelay - Win32 Debug with VSTGUI Source"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\source\Common\vstcontrols.h
# End Source File
# Begin Source File

SOURCE=..\source\common\vstgui.cpp

!IF  "$(CFG)" == "SurroundDelay - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SurroundDelay - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SurroundDelay - Win32 Debug with VSTGUI Source"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\source\Common\vstgui.h
# End Source File
# Begin Source File

SOURCE=.\library\vstgui.lib

!IF  "$(CFG)" == "SurroundDelay - Win32 Release"

!ELSEIF  "$(CFG)" == "SurroundDelay - Win32 Debug"

!ELSEIF  "$(CFG)" == "SurroundDelay - Win32 Debug with VSTGUI Source"

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

SOURCE=.\surrounddelay.def
# End Source File
# End Target
# End Project
