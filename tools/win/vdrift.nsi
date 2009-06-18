; basic script template for NSIS installers
;
; Written by Philip Chu
; Copyright (c) 2004-2005 Technicat, LLC
;
; This software is provided 'as-is', without any express or implied warranty.
; In no event will the authors be held liable for any damages arising from the use of this software.

; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it ; and redistribute
; it freely, subject to the following restrictions:

;    1. The origin of this software must not be misrepresented; you must not claim that
;       you wrote the original software. If you use this software in a product, an
;       acknowledgment in the product documentation would be appreciated but is not required.

;    2. Altered source versions must be plainly marked as such, and must not be
;       misrepresented as being the original software.

;    3. This notice may not be removed or altered from any source distribution.
; this file is edited and optimized to work for Vdrift

SetCompressor /FINAL /SOLID lzma
SetCompressorDictSize 64

!define version "2009-06-15"
!define setup "vdrift-setup-${version}.exe"

; change this to wherever the files to be packaged reside
!define srcdir "..\.."

;!define company "VDrift"

!define prodname "VDrift"
!define exec "vdrift.exe"
!define website "http:\\vdrift.net"

; optional stuff

; text file to open in notepad after installation
; !define notefile "docs\README"

; license text file
!define licensefile "docs\COPYING"

; icons must be Microsoft .ICO files
; !define icon "tools\win\VDrift.ico"

; installer background screen
; !define screenimage background.bmp

; file containing list of file-installation commands
; !define files "files.nsi"

; file containing list of file-uninstall commands
; !define unfiles "unfiles.nsi"
!define header "header.bmp"
!define welcome_installer "welcome-installer.bmp"
!define welcome_uninstaller "welcome-uninstaller.bmp"
; registry stuff

!define regkey "Software\${prodname}"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\${prodname}"

!define startmenu "$SMPROGRAMS\${prodname}"
!define uninstaller "uninstall.exe"

;--------------------------------
!include "MUI.nsh"
XPStyle on
ShowInstDetails hide
ShowUninstDetails hide

Name "${prodname}"
Caption "${prodname}"

!ifdef icon
	Icon "${srcdir}\${icon}"
!endif

OutFile "${setup}"

SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal

InstallDir "$PROGRAMFILES\${prodname}"
InstallDirRegKey HKLM "${regkey}" ""

;--------------------------------
;Interface Settings

!define MUI_ABORTWARNING

!define MUI_LICENSEPAGE_RADIOBUTTONS
  
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP ${header}
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP_NOSTRETCH
  
!define MUI_WELCOMEFINISHPAGE_BITMAP ${welcome_installer}
!define MUI_UNWELCOMEFINISHPAGE_BITMAP ${welcome_uninstaller}

;--------------------------------
;Pages

!insertmacro MUI_PAGE_WELCOME
!ifdef licensefile
	!insertmacro MUI_PAGE_LICENSE "${srcdir}\${licensefile}"
!endif
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\${exec}"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;Languages

!insertmacro MUI_LANGUAGE "English"



;--------------------------------

AutoCloseWindow false
ShowInstDetails show


; beginning (invisible) section
Section "Install VDrift" SEC_VDRIFT
	SectionIn RO ; The user can't disable it

	WriteRegStr HKLM "${regkey}" "Install_Dir" "$INSTDIR"
	; write uninstall strings
	WriteRegStr HKLM "${uninstkey}" "DisplayName" "${prodname} (remove only)"
	WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$INSTDIR\${uninstaller}"'

	!ifdef filetype
		WriteRegStr HKCR "${filetype}" "" "${prodname}"
	!endif

	WriteRegStr HKCR "${prodname}\Shell\open\command\" "" '"$INSTDIR\${exec} "%1"'

	!ifdef icon
		WriteRegStr HKCR "${prodname}\DefaultIcon" "" "$INSTDIR\${icon}"
	!endif

	SetOutPath $INSTDIR


	; package all files, recursively, preserving attributes
	; assume files are in the correct places
	File /a "${srcdir}\${exec}"
	File /a /oname=glew32.dll "${srcdir}\tools\win\dll\glew32.dll"
	File /a /oname=jpeg.dll "${srcdir}\tools\win\dll\jpeg.dll"
	File /a /oname=libpng12.dll "${srcdir}\tools\win\dll\libpng12.dll"
	File /a /oname=libtiff.dll "${srcdir}\tools\win\dll\libtiff.dll"
	File /a /oname=ogg.dll "${srcdir}\tools\win\dll\ogg.dll"
	File /a /oname=SDL.dll "${srcdir}\tools\win\dll\SDL.dll"
	File /a /oname=SDL_image.dll "${srcdir}\tools\win\dll\SDL_image.dll"
	File /a /oname=SDL_net.dll "${srcdir}\tools\win\dll\SDL_net.dll"
	File /a /oname=SDL_gfx.dll "${srcdir}\tools\win\dll\SDL_gfx.dll"
	File /a /oname=SDL_mixer.dll "${srcdir}\tools\win\dll\SDL_mixer.dll"
	File /a /oname=smpeg.dll "${srcdir}\tools\win\dll\smpeg.dll"
	File /a /oname=vorbis.dll "${srcdir}\tools\win\dll\vorbis.dll"
	File /a /oname=vorbisfile.dll "${srcdir}\tools\win\dll\vorbisfile.dll"
	File /a /oname=zlib1.dll "${srcdir}\tools\win\dll\zlib1.dll"

	File /a "${srcdir}\tools\win\VDrift.ico"

	!ifdef licensefile
		File /a /oname=license.txt "${srcdir}\${licensefile}"
	!endif

	!ifdef notefile
		File /a /oname=readme.txt "${srcdir}\${notefile}"
	!endif

	!ifdef icon
		File /a "${srcdir}\${icon}"
	!endif

	SetOutPath "$INSTDIR\data"
	     File /a /r /x .* /x SConscript /x vdrift-*x*.png /x cars /x tracks "${srcdir}\data\"
	SetOutPath "$INSTDIR\data\cars"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\360"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\CO"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\CS"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\F1-02"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\G4"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\M7"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\MC"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\MI"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\SV"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\T73"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\TC6"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\TL2"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\cars\XS"
     SetOutPath "$INSTDIR\data\tracks"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\tracks\bahrain"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\tracks\estoril88"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\tracks\hungaroring06"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\tracks\jerez88"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\tracks\monaco88"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\tracks\monza88"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\tracks\paulricard88"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\tracks\rouen"
		File /a /r /x .* /x SConscript /x vdrift-*x*.png "${srcdir}\data\tracks\vir"
	SetOutPath $INSTDIR

	File /a /oname=data\textures\icons\vdrift-16x16.png "${srcdir}\data\textures\icons\vdrift-16x16-windows.png"	
	File /a /oname=data\textures\icons\vdrift-32x32.png "${srcdir}\data\textures\icons\vdrift-32x32-windows.png"
	File /a /oname=data\textures\icons\vdrift-64x64.png "${srcdir}\data\textures\icons\vdrift-64x64-windows.png"
	;File /a /oname=data\tracks\track_list.txt "${srcdir}\data\tracks\track_list.txt.full"
	;File /a /oname=data\cars\car_list.txt "${srcdir}\data\cars\car_list.txt.full"
	; any application-specific files
	!ifdef files
		!include "${files}"
	!endif
	WriteUninstaller "${uninstaller}"
SectionEnd


; create shortcuts
Section "Create shortcuts" SEC_SHORTCUTS
	SectionSetText "shortcuts" "This adds shourtcuts on the desktop and in the Startmenu"
	CreateDirectory "${startmenu}"
	SetOutPath $INSTDIR ; for working directory
	!ifdef icon
		CreateShortCut "${startmenu}\${prodname}.lnk" "$INSTDIR\${exec}" "" "$INSTDIR\${icon}"
	!else
		CreateShortCut "${startmenu}\${prodname}.lnk" "$INSTDIR\${exec}"
	!endif

	CreateShortCut "${startmenu}\Uninstaller.lnk" "$INSTDIR\${uninstaller}"

	!ifdef notefile
		CreateShortCut "${startmenu}\Release Notes.lnk "$INSTDIR\${notefile}"
	!endif

	!ifdef helpfile
		CreateShortCut "${startmenu}\Documentation.lnk "$INSTDIR\${helpfile}"
	!endif

	!ifdef website
		WriteINIStr "${startmenu}\web site.url" "InternetShortcut" "URL" ${website}
		; CreateShortCut "${startmenu}\Web Site.lnk "${website}" "URL"
	!endif

	!ifdef notefile
		ExecShell "open" "$INSTDIR\${notefile}"
	!endif
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_VDRIFT} "The base VDrift package"
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_SHORTCUTS} "Adds shortcuts to the Desktop and startmenu"
!insertmacro MUI_FUNCTION_DESCRIPTION_END


; Uninstaller
; All section names prefixed by "Un" will be in the uninstaller
UninstallText "This will uninstall ${prodname}."
!ifdef icon
	UninstallIcon "${srcdir}\${icon}"
!endif

Section "Uninstall"
	DeleteRegKey HKLM "${uninstkey}"
	DeleteRegKey HKLM "${regkey}"

	Delete "${startmenu}\*.*"
	RMDir /r "${startmenu}\"

	!ifdef licensefile
		Delete "$INSTDIR\${licensefile}"
	!endif

	!ifdef notefile
		Delete "$INSTDIR\${notefile}"
	!endif

	!ifdef icon
		Delete "$INSTDIR\${icon}"
	!endif

	Delete "$INSTDIR\${exec}"
	Delete "$INSTDIR\*"
     RMDir /r "$INSTDIR\"
	!ifdef unfiles
		!include "${unfiles}"
	!endif
SectionEnd
