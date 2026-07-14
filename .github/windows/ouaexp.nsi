; Open UaExplorer Windows installer
;
; Packages the tree produced by `cmake --install` (or `build.ps1 -InstallPrefix <dir>`)
; into a signed-ready NSIS setup executable.
;
; Build:
;   makensis /DSTAGE_DIR=<staged install tree> ^
;            /DAPP_VERSION=1.0.0 ^
;            /DFILE_VERSION=1.0.0.0 ^
;            /DOUT_FILE=OpenUaExplorer-1.0.0-win64-setup.exe ^
;            .github\windows\ouaexp.nsi

Unicode true
ManifestDPIAware true

; Declare compatibility with every OS so that GetVersion (and therefore the
; ${AtLeastWin10} check below) reports the real Windows version instead of the
; 6.2 that an unmanifested process is fed on Windows 10 and later.
ManifestSupportedOS all

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "WinVer.nsh"
!include "x64.nsh"

;--------------------------------
; Configuration

!define PRODUCT_NAME      "Open UaExplorer"
!define PRODUCT_PUBLISHER "Ananev Alexandr"
!define PRODUCT_WEB_SITE  "https://github.com/sanny32/OpenUaExplorer"
!define EXE_NAME          "ouaexp.exe"

!define REG_APP_KEY       "Software\${PRODUCT_NAME}"
!define REG_UNINST_KEY    "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenUaExplorer"

; Session files. The icon is resource id 2 of the executable - see src/app/ouaexp.rc.in -
; which is the second icon in id order, hence the ",1" index the shell expects.
!define SESSION_EXT       ".ouas"
!define SESSION_PROGID    "OpenUaExplorer.Session"
!define SESSION_DESC      "Open UaExplorer Session"
!define SESSION_MIME      "application/x-ouaexp-session"
!define REG_CLASSES_KEY   "Software\Classes"

!ifndef SRC_DIR
    !define SRC_DIR "${__FILEDIR__}\..\.."
!endif

!ifndef STAGE_DIR
    !error "STAGE_DIR is not defined: pass /DSTAGE_DIR=<staged install tree>"
!endif

!ifndef APP_VERSION
    !error "APP_VERSION is not defined: pass /DAPP_VERSION=<display version, e.g. 1.0.0>"
!endif

!ifndef FILE_VERSION
    !error "FILE_VERSION is not defined: pass /DFILE_VERSION=<numeric version, e.g. 1.0.0.0>"
!endif

!ifndef OUT_FILE
    !define OUT_FILE "ouaexp-${APP_VERSION}-win64-setup.exe"
!endif

Name "${PRODUCT_NAME} ${APP_VERSION}"
OutFile "${OUT_FILE}"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${REG_APP_KEY}" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
BrandingText "${PRODUCT_NAME} ${APP_VERSION}"

;--------------------------------
; Version information

VIProductVersion "${FILE_VERSION}"
VIAddVersionKey "ProductName"     "${PRODUCT_NAME}"
VIAddVersionKey "ProductVersion"  "${APP_VERSION}"
VIAddVersionKey "FileVersion"     "${FILE_VERSION}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} Setup"
VIAddVersionKey "CompanyName"     "${PRODUCT_PUBLISHER}"
VIAddVersionKey "LegalCopyright"  "Copyright (C) ${PRODUCT_PUBLISHER}"

;--------------------------------
; Interface

!define MUI_ABORTWARNING
!define MUI_ICON   "${SRC_DIR}\src\app\res\icons\light\app.ico"
!define MUI_UNICON "${SRC_DIR}\src\app\res\icons\light\app.ico"

; 164x314 artwork shown on the Welcome and Finish pages of both the installer
; and the uninstaller.
!define MUI_WELCOMEFINISHPAGE_BITMAP   "${SRC_DIR}\.github\windows\nsis3-ouaexp.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${SRC_DIR}\.github\windows\nsis3-ouaexp.bmp"

!define MUI_FINISHPAGE_RUN "$INSTDIR\${EXE_NAME}"
!define MUI_FINISHPAGE_RUN_TEXT "Run ${PRODUCT_NAME}"
!define MUI_FINISHPAGE_LINK "Visit the project on GitHub"
!define MUI_FINISHPAGE_LINK_LOCATION "${PRODUCT_WEB_SITE}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${SRC_DIR}\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!define MUI_UNCONFIRMPAGE_TEXT_TOP "${PRODUCT_NAME} will be removed from the folder below."
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Installer

Function .onInit
    ${IfNot} ${AtLeastWin10}
        MessageBox MB_OK|MB_ICONSTOP "${PRODUCT_NAME} requires Windows 10 or later."
        Abort
    ${EndIf}

    ${IfNot} ${RunningX64}
        MessageBox MB_OK|MB_ICONSTOP "${PRODUCT_NAME} requires a 64-bit version of Windows."
        Abort
    ${EndIf}
    SetRegView 64

    ; Offer to remove a previously installed copy before overwriting it.
    ReadRegStr $R0 HKLM "${REG_UNINST_KEY}" "UninstallString"
    ${If} $R0 != ""
        ReadRegStr $R1 HKLM "${REG_UNINST_KEY}" "DisplayVersion"
        MessageBox MB_YESNO|MB_ICONQUESTION \
            "${PRODUCT_NAME} $R1 is already installed.$\n$\nUninstall it before continuing?" \
            IDNO +2
        ExecWait '$R0 /S _?=$INSTDIR'
    ${EndIf}
FunctionEnd

Section "${PRODUCT_NAME}" SEC_APP
    SectionIn RO
    SetShellVarContext all
    SetOutPath "$INSTDIR"

    File /r "${STAGE_DIR}\*.*"

    WriteUninstaller "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "${REG_APP_KEY}" "InstallDir" "$INSTDIR"

    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\${EXE_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk" "$INSTDIR\uninstall.exe"

    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "DisplayName"     "${PRODUCT_NAME}"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "DisplayVersion"  "${APP_VERSION}"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "DisplayIcon"     "$INSTDIR\${EXE_NAME}"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "URLInfoAbout"    "${PRODUCT_WEB_SITE}"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr   HKLM "${REG_UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
    WriteRegDWORD HKLM "${REG_UNINST_KEY}" "EstimatedSize"   "$0"
    WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoModify"        1
    WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoRepair"        1
SectionEnd

Section "Desktop shortcut" SEC_DESKTOP
    SetShellVarContext all
    CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\${EXE_NAME}"
SectionEnd

; The ProgID owns the icon and the open verb; the extension only names it. Writing
; both under HKLM\Software\Classes registers them for every user of the machine,
; which is what an installer running elevated is expected to do - a per-user default
; handler is the user's own choice to make and stays under HKCU.
Section "Associate ${SESSION_EXT} session files" SEC_ASSOC
    WriteRegStr HKLM "${REG_CLASSES_KEY}\${SESSION_PROGID}" "" "${SESSION_DESC}"
    WriteRegStr HKLM "${REG_CLASSES_KEY}\${SESSION_PROGID}\DefaultIcon" "" "$INSTDIR\${EXE_NAME},1"
    WriteRegStr HKLM "${REG_CLASSES_KEY}\${SESSION_PROGID}\shell\open\command" "" '"$INSTDIR\${EXE_NAME}" "%1"'

    WriteRegStr HKLM "${REG_CLASSES_KEY}\${SESSION_EXT}" "" "${SESSION_PROGID}"
    WriteRegStr HKLM "${REG_CLASSES_KEY}\${SESSION_EXT}" "Content Type" "${SESSION_MIME}"
    WriteRegStr HKLM "${REG_CLASSES_KEY}\${SESSION_EXT}" "PerceivedType" "document"
    WriteRegStr HKLM "${REG_CLASSES_KEY}\${SESSION_EXT}\OpenWithProgids" "${SESSION_PROGID}" ""

    ; Lets the shell offer the program in "Open with" for any file, and gives the
    ; entry the product name instead of the bare executable name.
    WriteRegStr HKLM "${REG_CLASSES_KEY}\Applications\${EXE_NAME}" "FriendlyAppName" "${PRODUCT_NAME}"
    WriteRegStr HKLM "${REG_CLASSES_KEY}\Applications\${EXE_NAME}\shell\open\command" "" '"$INSTDIR\${EXE_NAME}" "%1"'
    WriteRegStr HKLM "${REG_CLASSES_KEY}\Applications\${EXE_NAME}\SupportedTypes" "${SESSION_EXT}" ""

    ; The shell caches the icon of every known extension, so without this the new
    ; association keeps the generic icon until the cache happens to be rebuilt.
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_APP} "${PRODUCT_NAME} and the Qt runtime it needs."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "Place a shortcut on the desktop."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_ASSOC} "Open ${SESSION_EXT} session files with ${PRODUCT_NAME} and show them with its icon."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller

Function un.onInit
    SetRegView 64
FunctionEnd

Section "Uninstall"
    SetShellVarContext all

    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk"
    RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

    ; The install tree is self-contained (exe, Qt runtime, plugins), so it is
    ; removed wholesale. Settings under HKCU are left untouched on purpose.
    Delete "$INSTDIR\uninstall.exe"
    RMDir /r "$INSTDIR\plugins"
    RMDir /r "$INSTDIR"

    ; The extension key is only dropped while it still names this ProgID: another
    ; program may have taken the type over since, and that claim must survive.
    ReadRegStr $R0 HKLM "${REG_CLASSES_KEY}\${SESSION_EXT}" ""
    ${If} $R0 == "${SESSION_PROGID}"
        DeleteRegKey HKLM "${REG_CLASSES_KEY}\${SESSION_EXT}"
    ${Else}
        DeleteRegValue HKLM "${REG_CLASSES_KEY}\${SESSION_EXT}\OpenWithProgids" "${SESSION_PROGID}"
    ${EndIf}

    DeleteRegKey HKLM "${REG_CLASSES_KEY}\${SESSION_PROGID}"
    DeleteRegKey HKLM "${REG_CLASSES_KEY}\Applications\${EXE_NAME}"

    DeleteRegKey HKLM "${REG_UNINST_KEY}"
    DeleteRegKey HKLM "${REG_APP_KEY}"

    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
SectionEnd
