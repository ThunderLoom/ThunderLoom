!include MUI2.nsh
!include "FileFunc.nsh"

Name "Thunderloom 3ds Max"
OutFile "thunderLoomVRay3dsMax.exe"

RequestExecutionLevel admin
Unicode True
;SetCompressor lzma
SetDateSave off

InstallDir $APPDATA\Autodesk\ApplicationPlugins\thunderloom_3ds_max
;--------------------------------

; Pages
!define MUI_WELCOMEFINISHPAGE_BITMAP "installer_sidebar.bmp"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of the Thunderloom plugin for 3ds Max"
!insertmacro MUI_PAGE_WELCOME

;!define MUI_HEADERIMAGE
;!define MUI_HEADERIMAGE_BITMAP "header.bmp"
;!define MUI_HEADERIMAGE_BITMAP_STRETCH FitControl
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_TEXT "Thunderloom for 3ds Max has been successfully installed and will be available the next time you start 3ds Max."
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Thunderloom"

Section ""
    SetOutPath $INSTDIR
    File ..\thunderLoomVRay3dsMax\*
    WriteUninstaller "$INSTDIR\uninstall_thunderloom_3ds_max.exe"
    WriteRegStr HKCU ${UNINSTKEY} "DisplayName" "Thunderloom for 3ds Max"
    WriteRegStr HKCU ${UNINSTKEY} "UninstallString" "$\"$INSTDIR\uninstall_thunderloom_3ds_max.exe$\""
    WriteRegStr HKCU ${UNINSTKEY} "Publisher" "Thunderloom"
    WriteRegStr HKCU ${UNINSTKEY} "InstallLocation" "$INSTDIR"
    WriteRegDWORD HKCU ${UNINSTKEY} "NoModify" 1
    WriteRegDWORD HKCU ${UNINSTKEY} "NoRepair" 1
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKCU "${UNINSTKEY}" "EstimatedSize" "$0"
    WriteRegDWORD HKCU "${UNINSTKEY}" "VersionMajor" 1
    WriteRegDWORD HKCU "${UNINSTKEY}" "VersionMinor" 0
    WriteRegStr HKCU "${UNINSTKEY}" "DisplayVersion" "1.0"
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\*"
    RMDir "$INSTDIR"
    DeleteRegKey HKCU ${UNINSTKEY}
SectionEnd
