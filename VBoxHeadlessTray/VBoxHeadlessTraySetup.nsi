;VBoxHeadlessTray NSIS Installation Script

!define prodname "VBoxHeadlessTray"
!define coname "Topten Software"
!define outfile ".\VBoxHeadlessTraySetup.exe"
!define licencefile "VBoxHeadless_License.txt"
!define appexe "SnapShot.exe"
!define produrl "http://www.toptensoftware.com/VBoxHeadlessTray"

;--------------------------------
;Include Modern UI

!include "MUI.nsh"
!include "LogicLib.nsh"

;--------------------------------
;General

;Name and file
Name "${prodname}"
OutFile "${outfile}"

;Default installation folder
InstallDir "$PROGRAMFILES64\${coname}\${prodname}"

;Get installation folder from registry if available
InstallDirRegKey HKLM "Software\${coname}\InstallDir" "${prodname}"

;--------------------------------
;Interface Settings

!define MUI_ABORTWARNING

;--------------------------------
;Pages

!insertmacro MUI_PAGE_LICENSE "${licencefile}"
;!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

!insertmacro MUI_LANGUAGE "English"


;--------------------------------
;Installer Sections

Section "${prodname}" SecMain

	SetShellVarContext all

	SetOutPath "$INSTDIR"

	${If} $PROGRAMFILES64 S== $PROGRAMFILES32
		File ".\Win32\Release\VBoxHeadlessTray.exe"
	${Else}
		File /oname=VBoxHeadlessTray.exe ".\x64\Release\VBoxHeadlessTray64.exe"
	${Endif}
	
	;Store installation folder
	WriteRegStr HKLM "Software\${coname}\InstallDir" "${prodname}" $INSTDIR
	
	; Create shortcut
	CreateShortCut "$SMPROGRAMS\${prodname}.lnk" "$INSTDIR\VBoxHeadlessTray.exe"

	; Update Add/Remove Programs
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}" 
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}" "DisplayName" "${prodname}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}" "UninstallString" "$INSTDIR\Uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}" "DisplayIcon" "$INSTDIR\${appexe},0"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}" "Publisher" "${coname}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}" "URLInfoAbout" "${produrl}"
	WriteRegDWord HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}" "NoRepair" 1
	WriteRegDWord HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}" "NoModify" 1
	

	;Create uninstaller
	WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

;--------------------------------
;Descriptions

;;Language strings
;LangString DESC_SecMain ${LANG_ENGLISH} "${prodname}"
;
;;Assign language strings to sections
;!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
;	!insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
;!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

	SetShellVarContext all

	; Remove files
	Delete "$INSTDIR\VBoxHeadlessTray.exe"
	Delete "$INSTDIR\Uninstall.exe"

	; Remove short cut	
	Delete "$SMPROGRAMS\${prodname}.lnk"

	; Remove installation folder
	RMDir "$INSTDIR"

	; Remove from Add/Remove Programs
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}" 

	; Remove installation folder
	DeleteRegValue HKLM "Software\${coname}\InstallDir" "${prodname}"
	DeleteRegKey /ifempty HKLM "Software\${coname}\InstallDir"
	DeleteRegKey /ifempty HKLM "Software\${coname}"

SectionEnd