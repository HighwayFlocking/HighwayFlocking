; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "HighwayFlocking"
#define MyAppVersion "1.0"
#define MyAppPublisher "NTNU"
#define MyAppURL "https://highwayflocking.github.io/"
#define MyAppExeName "gui.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{9641E0B8-031A-4486-A0F3-4DF0C4F725B6}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
AppCopyright=Sindre Johansen and Andreas L�vland
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=Setup/
OutputBaseFilename=HighwayFlockingSetup
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked

[Files]
Source: dist\PySide.QtGui.pyd; DestDir: {app}
Source: dist\PySide.QtCore.pyd; DestDir: {app}
Source: dist\pyexpat.pyd; DestDir: {app}
Source: dist\msgpack._unpacker.pyd; DestDir: {app}
Source: dist\msgpack._packer.pyd; DestDir: {app}
Source: dist\library.zip; DestDir: {app}
Source: dist\gui.log; DestDir: {app}
Source: dist\gui.exe; DestDir: {app}
Source: dist\_ssl.pyd; DestDir: {app}
Source: dist\_socket.pyd; DestDir: {app}
Source: dist\_lzma.pyd; DestDir: {app}
Source: dist\_hashlib.pyd; DestDir: {app}
Source: dist\_ctypes.pyd; DestDir: {app}
Source: dist\_bz2.pyd; DestDir: {app}
Source: dist\unicodedata.pyd; DestDir: {app}
Source: dist\shiboken-python3.4.dll; DestDir: {app}
Source: dist\select.pyd; DestDir: {app}
Source: dist\QtNetwork4.dll; DestDir: {app}
Source: dist\QtGui4.dll; DestDir: {app}
Source: dist\QtCore4.dll; DestDir: {app}
Source: dist\python34.dll; DestDir: {app}
Source: dist\pyside-python3.4.dll; DestDir: {app}
Source: dist\PySide.QtNetwork.pyd; DestDir: {app}
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\Ogg\Win64\VS2013\libogg_64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\Ogg\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013\ApexFrameworkPROFILE_x64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013\APEX_ClothingPROFILE_x64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013\APEX_DestructiblePROFILE_x64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013\APEX_LegacyPROFILE_x64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013\APEX_LoaderPROFILE_x64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013\APEX_ParticlesPROFILE_x64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64\VS2013\nvToolsExt64_1.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64\VS2013\PhysX3CommonPROFILE_x64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64\VS2013\PhysX3CookingPROFILE_x64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64\VS2013\PhysX3GpuPROFILE_x64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64\VS2013\PhysX3PROFILE_x64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\Vorbis\Win64\VS2013\libvorbisfile_64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\Vorbis\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Binaries\ThirdParty\Vorbis\Win64\VS2013\libvorbis_64.dll; DestDir: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\Vorbis\Win64\VS2013
Source: dist\WindowsNoEditor\Engine\Build\build.properties; DestDir: {app}\WindowsNoEditor\Engine\Build
Source: dist\WindowsNoEditor\Engine\Extras\Redist\en-us\UE4PrereqSetup_x64.exe; DestDir: {app}\WindowsNoEditor\Engine\Extras\Redist\en-us
Source: dist\WindowsNoEditor\HighwayFlocking\Binaries\Win64\HighwayFlocking.exe; DestDir: {app}\WindowsNoEditor\HighwayFlocking\Binaries\Win64
Source: dist\WindowsNoEditor\HighwayFlocking\Content\Paks\HighwayFlocking-WindowsNoEditor.pak; DestDir: {app}\WindowsNoEditor\HighwayFlocking\Content\Paks
Source: dist\WindowsNoEditor\Manifest_NonUFSFiles.txt; DestDir: {app}\WindowsNoEditor\
Source: dist\WindowsNoEditor\HighwayFlocking.exe; DestDir: {app}\WindowsNoEditor\
Source: Setup\vcredist_x86.exe; DestDir: {tmp}; Flags: deleteafterinstall nocompression
[Icons]
Name: {group}\{#MyAppName}; Filename: {app}\{#MyAppExeName}
Name: {group}\{cm:UninstallProgram,{#MyAppName}}; Filename: {uninstallexe}
Name: {commondesktop}\{#MyAppName}; Filename: {app}\{#MyAppExeName}; Tasks: desktopicon

[Run]
Filename: {app}\{#MyAppExeName}; Description: {cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}; Flags: nowait postinstall skipifsilent
Filename: {tmp}\vcredist_x86.exe; Parameters: /Q; StatusMsg: Installing Microsoft Visual C++ 2008 Redistributable Package (x86); Flags: skipifdoesntexist; Languages:
[Dirs]
Name: {app}\WindowsNoEditor
Name: {app}\WindowsNoEditor\Engine
Name: {app}\WindowsNoEditor\Engine\Binaries
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\Ogg
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\Ogg\Win64
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\Ogg\Win64\VS2013
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\APEX-1.3\Win64\VS2013
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\PhysX\PhysX-3.3\Win64\VS2013
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\Vorbis
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\Vorbis\Win64
Name: {app}\WindowsNoEditor\Engine\Binaries\ThirdParty\Vorbis\Win64\VS2013
Name: {app}\WindowsNoEditor\Engine\Build
Name: {app}\WindowsNoEditor\Engine\Extras
Name: {app}\WindowsNoEditor\Engine\Extras\Redist
Name: {app}\WindowsNoEditor\Engine\Extras\Redist\en-us
Name: {app}\WindowsNoEditor\HighwayFlocking
Name: {app}\WindowsNoEditor\HighwayFlocking\Binaries
Name: {app}\WindowsNoEditor\HighwayFlocking\Binaries\Win64
Name: {app}\WindowsNoEditor\HighwayFlocking\Content
Name: {app}\WindowsNoEditor\HighwayFlocking\Content\Paks
