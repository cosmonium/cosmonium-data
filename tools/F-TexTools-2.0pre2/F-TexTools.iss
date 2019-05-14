; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=F-TexTools
AppVerName=F-TexTools-2.0pre2
AppPublisher=Dr. F. Schrempp
AppPublisherURL=http://www.celestialmatters.org
AppSupportURL=http://www.celestialmatters.org
AppUpdatesURL=http://www.celestialmatters.org
; DefaultDirName={pf}\F-TxTools
DefaultDirName={reg:HKLM\Software\F-TexTools,Path|{pf}\F-TexTools}
DefaultGroupName=F-TexTools
InfoAfterFile=README
OutputDir=Win32_PC.bin
OutputBaseFilename=F-TexTools-2.0pre2
LicenseFile=COPYING
Compression=lzma
SolidCompression=yes


[Languages]
Name: eng; MessagesFile: compiler:Default.isl
Name: fre; MessagesFile: compiler:Languages\French.isl
Name: ger; MessagesFile: compiler:Languages\German.isl
Name: nor; MessagesFile: compiler:Languages\Norwegian.isl
Name: rus; MessagesFile: compiler:Languages\Russian.isl

[Files]
; Executables
Source: Win32_PC.bin\tx2pow2.exe; DestDir: {app}; Flags: ignoreversion
Source: Win32_PC.bin\tx2half.exe; DestDir: {app}; Flags: ignoreversion
Source: Win32_PC.bin\bin2png.exe; DestDir: {app}; Flags: ignoreversion
Source: Win32_PC.bin\png2bin.exe; DestDir: {app}; Flags: ignoreversion
Source: Win32_PC.bin\txtiles.exe; DestDir: {app}; Flags: ignoreversion
Source: Win32_PC.bin\txtilesDXT.exe; DestDir: {app}; Flags: ignoreversion
Source: Win32_PC.bin\specmap.exe; DestDir: {app}; Flags: ignoreversion
Source: Win32_PC.bin\tx2rgba.exe; DestDir: {app}; Flags: ignoreversion
Source: Win32_PC.bin\libpng13.dll; DestDir: {app}; Flags: ignoreversion
Source: Win32_PC.bin\zlib1.dll; DestDir: {app}; Flags: ignoreversion
Source: Win32_PC.bin\nvtt.dll; DestDir: {app}; Flags: ignoreversion
Source: Win32_PC.bin\cudart.dll; DestDir: {app}; Flags: ignoreversion
; Text files
Source: AUTHORS; DestDir: {app}; Flags: ignoreversion; DestName: AUTHORS.txt
Source: README.doc; DestDir: {app}; Flags: ignoreversion
Source: COPYING; DestDir: {app}; Flags: ignoreversion
Source: ChangeLog.doc; DestDir: {app}; Flags: ignoreversion
Source: NVIDIA_Texture_Tools_LICENSE.txt; DestDir: {app}; Flags: ignoreversion
; Script files
;Source: src\shell-scripts\dorgb; DestDir: {app}; Flags: ignoreversion
;Source: src\shell-scripts\dorgba; DestDir: {app}; Flags: ignoreversion
;Source: src\shell-scripts\dospec; DestDir: {app}; Flags: ignoreversion

[Registry]
; The Software\F-TexTools key is created by the Celestia program, so it needs
; to be deleted during an uninstall.
Root: HKCU; Subkey: Software\F-TexTools; Flags: uninsdeletekey
Root: HKCU; SubKey: Environment; ValueType: expandsz; Valuename: Path; Valuedata: "{olddata};{app}"; Flags: uninsdeletekeyifempty

; NOTE: Don't use "Flags: ignoreversion" on any shared system files
