; Dynamic Windows Interface Builder NSIS Modern User Interface

;--------------------------------
; Include Modern UI

  !include "MUI2.nsh"

;--------------------------------
; General

  ; Name and file
  Name "Dynamic Windows Interface Builder"
  OutFile "dwib10b5win.exe"

  ; Default installation folder
  InstallDir "$PROGRAMFILES\DWIB"
  
  ; Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\DWIB" ""

  ; Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
; Variables

  Var StartMenuFolder

;--------------------------------
; Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
; Pages

  !insertmacro MUI_PAGE_LICENSE "..\scripts\license.txt"
  ;!insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\DWIB" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
  
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
; Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Installer Sections

Section "Dummy Section" SecDummy

  SetOutPath "$INSTDIR"

  ; Binaries
  File dwib.exe
  File *.dll
  ; Developer Header
  File dwib.h
  ; Developer Libraries
  File /r x86
  File /nonfatal /r x64
  ; Help
  File readme.txt

  ; Store installation folder
  WriteRegStr HKCU "Software\DWIB" "" $INSTDIR

  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

    ; Create shortcuts
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Dynamic Windows Interface Builder.lnk" "$INSTDIR\dwib.exe"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

    ; Create Uninstall infor 
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWIB" \
                     "DisplayName" "Dynamic Windows Interface Builder"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWIB" \
                     "UninstallString" "$\"$INSTDIR\uninstall.exe$\""  
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

;--------------------------------
; Descriptions

  ; Language strings
  ;LangString DESC_SecDummy ${LANG_ENGLISH} "A test section."

  ; Assign language strings to sections
  ;!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  ;  !insertmacro MUI_DESCRIPTION_TEXT ${SecDummy} $(DESC_SecDummy)
  ;!insertmacro MUI_FUNCTION_DESCRIPTION_END
 
;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ; Binaries
  Delete "$INSTDIR\dwib.exe"
  Delete "$INSTDIR\*.dll"
  ; Developer Header
  Delete "$INSTDIR\dwib.h"
  ; Developer Libraries
  Delete "$INSTDIR\x86\dwib.dll"
  Delete "$INSTDIR\x86\dwib.lib"
  Delete "$INSTDIR\x64\dwib.dll"
  Delete "$INSTDIR\x64\dwib.lib"
  ; Help
  Delete "$INSTDIR\readme.txt"

  Delete "$INSTDIR\Uninstall.exe"

  ; Directories
  RMDir "$INSTDIR\x86"
  RMDir "$INSTDIR\x64"
  RMDir "$INSTDIR"
  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
    
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Dynamic Windows Interface Builder.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"
  
  DeleteRegKey /ifempty HKCU "Software\DWIB"

SectionEnd