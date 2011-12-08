# Microsoft Developer Studio Project File - Name="aoqsf" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=aoqsf - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aoqsf.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aoqsf.mak" CFG="aoqsf - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aoqsf - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "aoqsf - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "aoqsf - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_aoqsf"
# PROP Intermediate_Dir "Release_aoqsf"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AODSF_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Zi /O2 /I "src/aosdk" /I "src/zlib" /I "./src/aosdk" /D "LSB_FIRST" /D "NDEBUG" /D "AODSF_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_CRT_SECURE_NO_DEPRECATE" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:none /debug /machine:I386 /out:"aoqsf.bin"
# Begin Custom Build
TargetPath=.\aoqsf.bin
TargetName=aoqsf
InputPath=.\aoqsf.bin
SOURCE="$(InputPath)"

"C:\Program Files\Winamp\Plugins\$(TargetName).bin" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(TargetPath)" "C:\Program Files\Winamp\Plugins\$(TargetName).bin"

# End Custom Build

!ELSEIF  "$(CFG)" == "aoqsf - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_aoqsf"
# PROP Intermediate_Dir "Debug_aoqsf"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AODSF_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "src/aosdk" /I "src/zlib" /D "LSB_FIRST" /D "_DEBUG" /D "AODSF_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_CRT_SECURE_NO_DEPRECATE" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"C:\usr\bin\kbmed242_beta4\Plugins/aoqsf.bin" /pdbtype:sept
# Begin Custom Build
TargetPath=\usr\bin\kbmed242_beta4\Plugins\aoqsf.bin
TargetName=aoqsf
InputPath=\usr\bin\kbmed242_beta4\Plugins\aoqsf.bin
SOURCE="$(InputPath)"

"C:\Program Files\Winamp\Plugins\$(TargetName).bin" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(TargetPath)" "C:\Program Files\Winamp\Plugins\$(TargetName).bin"

# End Custom Build

!ENDIF 

# Begin Target

# Name "aoqsf - Win32 Release"
# Name "aoqsf - Win32 Debug"
# Begin Group "aosdk"

# PROP Default_Filter ""
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=.\src\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=.\src\zlib\crc32.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\crypt.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\infback.c
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inflate.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=.\src\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=.\src\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\zutil.c
# End Source File
# Begin Source File

SOURCE=.\src\zlib\zutil.h
# End Source File
# End Group
# Begin Group "eng_qsf"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aosdk\eng_qsf\eng_qsf.c
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\eng_qsf\kabuki.c
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\eng_qsf\mem.c
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\eng_qsf\mem.h
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\eng_qsf\qsound.c
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\eng_qsf\qsound.h
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\eng_qsf\z80.c
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\eng_qsf\z80.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\aosdk\ao.h
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\corlett.c
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\corlett.h
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\cpuintrf.h
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\eng_protos.h
# End Source File
# Begin Source File

SOURCE=.\src\aosdk\osd_cpu.h
# End Source File
# End Group
# Begin Group "xsfc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\xsfc\xsfdrv.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\aoqsf.c
# End Source File
# Begin Source File

SOURCE=.\src\pversion.h
# End Source File
# End Target
# End Project
