#ifndef AppVersion
  #error AppVersion must come from VERSION via scripts/build.ps1
#endif
[Setup]
AppId={{A5DCB49F-5BED-4404-9827-85BF0BD5DEB0}
AppName=Basilisk Battery
AppVersion={#AppVersion}
AppPublisher=luingry
AppPublisherURL=https://github.com/luingry/razer-bzlskv3pro-battery-tracker
DefaultDirName={localappdata}\Programs\Basilisk Battery
DefaultGroupName=Basilisk Battery
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir=..\artifacts
OutputBaseFilename=BasiliskBattery-Setup-{#AppVersion}
SetupIconFile=..\assets\app.ico
UninstallDisplayIcon={app}\BasiliskBattery.exe
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no
AppMutex=Local\BasiliskBattery.SingleInstance
UninstallDisplayName=Basilisk Battery

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Files]
Source: "..\build\BasiliskBattery.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Basilisk Battery"; Filename: "{app}\BasiliskBattery.exe"

[Run]
Filename: "{app}\BasiliskBattery.exe"; Description: "Iniciar Basilisk Battery"; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueName: "BasiliskBattery"; Flags: uninsdeletevalue

[Code]
procedure CloseRunningApp();
var
  Window: HWND;
begin
  Window := FindWindowByClassName('BasiliskBattery.Window');
  if Window <> 0 then begin
    PostMessage(Window, $0010, 0, 0);
    Sleep(500);
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  CloseRunningApp();
  Result := '';
end;

function InitializeUninstall(): Boolean;
begin
  CloseRunningApp();
  Result := True;
end;
