# Microsoft Developer Studio Project File - Name="vrayblinnmtl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=vrayblinnmtl - Win32 Max Release Official
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vrayblinnmtl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vrayblinnmtl.mak" CFG="vrayblinnmtl - Win32 Max Release Official"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vrayblinnmtl - Win32 Max Release Official" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vrayblinnmtl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vrayblinnmtl - Win32 Max Release Official"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vrayblinnmtl___Win32_Official_Max_6"
# PROP BASE Intermediate_Dir "vrayblinnmtl___Win32_Official_Max_6"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\build\vrayblinnmtl\max90\x64\official"
# PROP Intermediate_Dir "..\..\build\vrayblinnmtl\max90\x64\official"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MD /W3 /GX /O2 /I "k:\3dsmax\maxsdk90\include" /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "vrayblinnmtl_EXPORTS" /YX /FD /Qprec_div /c
# ADD CPP /nologo /G6 /MD /W3 /GX /Zi /O2 /I "k:\3dsmax\maxsdk90\include" /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "vrayblinnmtl_EXPORTS" /YX /FD /Qprec_div /Qvc7 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 paramblk2.lib core.lib geom.lib maxutil.lib bmm.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x08B70000" /dll /machine:AMD64 /out:"..\..\vrayplugins\vrayblinnmtl90.dlt" /libpath:"k:\3dsmax\maxsdk90\lib"  /release
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 paramblk2.lib core.lib geom.lib maxutil.lib bmm.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib vray90.lib vutils_s.lib plugman_s.lib vrender90.lib /base:"0x08B70000" /dll /pdb:"..\..\pdb\vrayblinnmtl90.pdb" /debug /machine:AMD64 /def:".\plugin.def" /out:"..\..\vrayplugins\vrayblinnmtl90.dlt" /libpath:"k:\3dsmax\maxsdk90\Lib\x64" /libpath:"..\..\..\lib\x64" /libpath:"..\..\..\lib\x64\vc8" /release
# SUBTRACT LINK32 /nologo /pdb:none

!ELSEIF  "$(CFG)" == "vrayblinnmtl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vrayblinnmtl___Win32_Max6_Release"
# PROP BASE Intermediate_Dir "vrayblinnmtl___Win32_Max6_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\build\vrayblinnmtl\debug\x64"
# PROP Intermediate_Dir "..\..\build\vrayblinnmtl\debug\x64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MD /W3 /GX /O2 /I "k:\3dsmax\maxsdk90\include" /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "vrayblinnmtl_EXPORTS" /YX /FD /Qprec_div /c
# ADD CPP /nologo /G6 /MD /W3 /GX /O2 /I "k:\3dsmax\maxsdk90\include" /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "vrayblinnmtl_EXPORTS" /YX /FD /Qprec_div /Qvc7 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 paramblk2.lib core.lib geom.lib maxutil.lib bmm.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x08B70000" /dll /machine:AMD64 /out:"o:\vray\1.0\currentbuild\x64\adv\vrayblinnmtl90.dlt" /libpath:"k:\3dsmax\maxsdk90\Lib" /libpath:"..\..\..\lib\x64" /libpath:"..\..\..\lib\x64\vc8"  /release
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 paramblk2.lib core.lib geom.lib maxutil.lib bmm.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib vray90.lib vutils_s.lib plugman_s.lib /nologo /base:"0x08B70000" /dll /machine:AMD64 /out:"w:\program files\max80\plugins\vrayplugins\vrayblinnmtl90.dlt" /libpath:"k:\3dsmax\maxsdk90\Lib" /libpath:"..\..\..\lib\x64" /libpath:"..\..\..\lib\x64\vc8"  /release
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "vrayblinnmtl - Win32 Max Release Official"
# Name "vrayblinnmtl - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\blinnbrdf.cpp
# End Source File
# Begin Source File

SOURCE=.\mtlshade.cpp
# End Source File
# Begin Source File

SOURCE=.\plugin.cpp
# End Source File
# Begin Source File

SOURCE=.\plugin.def

!IF  "$(CFG)" == "vrayblinnmtl - Win32 Max Release Official"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vrayblinnmtl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vrayblinnmtl.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\blinnbrdf.h
# End Source File
# Begin Source File

SOURCE=.\vrayblinnmtl.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\common\powershader.bmp
# End Source File
# Begin Source File

SOURCE=.\vrayblinnmtl.rc
# End Source File
# End Group
# Begin Group "ReadMe"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Skeleton.txt
# End Source File
# End Group
# End Target
# End Project
