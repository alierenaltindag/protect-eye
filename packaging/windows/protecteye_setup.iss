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

[CustomMessages]
turkish.PreviousInstallPrompt=Sistemde mevcut bir ProtectEye kurulumu tespit edildi.%n%nTemiz bir kurulum için eski sürüm tamamen kaldırılsın mı?
english.PreviousInstallPrompt=An existing ProtectEye installation was detected.%n%nWould you like to completely remove the previous version before continuing?
turkish.RemoveSettingsPrompt=ProtectEye kullanıcı ayarlarını ve yapılandırma verilerini de silmek istiyor musunuz?
english.RemoveSettingsPrompt=Do you want to remove ProtectEye settings and configuration data as well?

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
; Unconditionally delete Run value on uninstall even if enabled later via in-app settings
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueName: "{#MyAppName}"; Flags: dontcreatekey uninsdeletevalue

[Run]
; Launch ProtectEye automatically after install (no checkbox, runs silently in tray)
Filename: "{app}\{#MyAppExeName}"; Flags: nowait runasoriginaluser

[UninstallRun]
; Terminate running ProtectEye instance silently before file removal
Filename: "taskkill.exe"; Parameters: "/F /IM {#MyAppExeName}"; Flags: runhidden; RunOnceId: "KillApp"

[UninstallDelete]
; Clean up entire application directory and shortcuts even if runtime files were created
Type: filesandordirs; Name: "{app}"
Type: files; Name: "{autodesktop}\{#MyAppName}.lnk"
Type: filesandordirs; Name: "{autoprograms}\{#MyAppName}"

[Code]
// Close running instance at the very beginning of uninstallation
function InitializeUninstall(): Boolean;
var
  ErrorCode: Integer;
begin
  Exec('taskkill.exe', '/F /IM {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
  Sleep(500);
  Result := True;
end;

// Post-uninstallation cleanup: remove autostart entry and prompt for settings wipe
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    // Unconditionally remove autostart registry entry
    RegDeleteValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', '{#MyAppName}');

    // Clean any temporary lock files
    DeleteFile(ExpandConstant('{tmp}\protecteye_*.lock'));

    // Prompt user to delete user settings / configuration from registry
    if SuppressibleMsgBox(CustomMessage('RemoveSettingsPrompt'), mbConfirmation, MB_YESNO, IDYES) = IDYES then
    begin
      RegDeleteKeyIncludingSubkeys(HKCU, 'Software\{#MyAppName}');
    end;
  end;
end;

// Check for existing installation
function GetPreviousUninstallString(): String;
var
  sUninstPath: String;
begin
  sUninstPath := '';
  // Check HKCU (PrivilegesRequired=lowest) and fallback to HKLM
  if not RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{D9A83F42-B35E-4F28-8C61-B65E478F12A0}_is1', 'UninstallString', sUninstPath) then
    RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{D9A83F42-B35E-4F28-8C61-B65E478F12A0}_is1', 'UninstallString', sUninstPath);
  Result := sUninstPath;
end;

// Close running instance and prompt user for clean installation if previous install is found
function InitializeSetup(): Boolean;
var
  ErrorCode: Integer;
  OldUninstallString: String;
begin
  // Terminate running ProtectEye instance silently
  Exec('taskkill.exe', '/F /IM {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);

  OldUninstallString := GetPreviousUninstallString();
  if OldUninstallString <> '' then
  begin
    OldUninstallString := RemoveQuotes(OldUninstallString);
    if FileExists(OldUninstallString) then
    begin
      if SuppressibleMsgBox(CustomMessage('PreviousInstallPrompt'), mbConfirmation, MB_YESNO, IDYES) = IDYES then
      begin
        // Run uninstaller silently and wait for completion before proceeding
        Exec(OldUninstallString, '/SILENT /NORESTART', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
      end;
    end;
  end;

  Result := True;
end;

