; ProtectEye Inno Setup Installer Script
; Produces: ProtectEye_Setup.exe
; Version is passed at compile time via: ISCC.exe /DMyAppVersion=X.Y.Z
; Falls back to 1.0.0 for local manual builds.

#define MyAppName "ProtectEye"
#ifndef MyAppVersion
  #define MyAppVersion "1.0.3"
#endif



#define MyAppPublisher "ProtectEye Team"
#define MyAppURL "https://github.com/alierenaltindag/protect-eye"
#define MyAppExeName "protecteye.exe"


[Setup]
AppId={{D9A83F42-B35E-4F28-8C61-B65E478F12A0}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputDir=..\..\build\windows_installer
OutputBaseFilename=ProtectEye_Setup
SetupIconFile=..\..\resources\app_icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
CloseApplications=yes
RestartApplications=no


[Languages]
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autostart"; Description: "Windows başladığında otomatik çalıştır (Önerilir)"; GroupDescription: "Başlangıç Ayarları:"

[Files]
; Dist directory containing protecteye.exe and windeployqt output
Source: "..\..\build\dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#MyAppExeName}"
Name: "{autoprograms}\{#MyAppName}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; Automatic startup on Windows boot via HKCU Run
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "{#MyAppName}"; ValueData: """{app}\{#MyAppExeName}"" --autostart"; Flags: uninsdeletevalue; Tasks: autostart

[Run]
; Launch ProtectEye automatically after install (no checkbox, runs silently in tray)
Filename: "{app}\{#MyAppExeName}"; Flags: nowait runasoriginaluser

[UninstallRun]
; Terminate running ProtectEye instance silently before file removal
Filename: "taskkill.exe"; Parameters: "/F /IM {#MyAppExeName}"; Flags: runhidden; RunOnceId: "KillApp"

[Code]
// Close running instance at the very beginning of uninstallation
function InitializeUninstall(): Boolean;
var
  ErrorCode: Integer;
begin
  Exec('taskkill.exe', '/F /IM {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
  Result := True;
end;

// Close running instance at the very beginning of installation/update
function InitializeSetup(): Boolean;
var
  ErrorCode: Integer;
begin
  Exec('taskkill.exe', '/F /IM {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
  Result := True;
end;

