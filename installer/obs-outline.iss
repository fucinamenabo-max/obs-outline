; Installer di "Outline Filter" per OBS Studio (Windows x64).
; Compilare con Inno Setup 6:  ISCC.exe installer\obs-outline.iss
; Prima compilare il plugin in Release (cartella build_vs17\rundir\Release).

#define MyName "Outline Filter per OBS"
#define MyVersion "1.0.3"
#define MyPublisher "Fucina Menabò"
#define MyUrl "https://fucinamenabo.com"

[Setup]
AppId={{6B1F2E5A-7C3D-4A8E-9B64-0F5E2D1A9C37}
AppName={#MyName}
AppVersion={#MyVersion}
AppPublisher={#MyPublisher}
AppPublisherURL={#MyUrl}
DefaultDirName={code:GetObsDir}
DirExistsWarning=no
DisableProgramGroupPage=yes
DisableReadyPage=no
UsePreviousAppDir=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=obs-outline-{#MyVersion}-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyName}
AppendDefaultDirName=no

[Languages]
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"

[Files]
Source: "..\build_vs17\rundir\Release\obs-outline.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion
Source: "..\data\*"; DestDir: "{app}\data\obs-plugins\obs-outline"; Flags: ignoreversion recursesubdirs createallsubdirs

[Messages]
italian.SelectDirLabel3=Seleziona la cartella di installazione di OBS Studio (quella che contiene "bin" e "obs-plugins").
english.SelectDirLabel3=Select the OBS Studio installation folder (the one that contains "bin" and "obs-plugins").
spanish.SelectDirLabel3=Selecciona la carpeta de instalación de OBS Studio (la que contiene "bin" y "obs-plugins").
french.SelectDirLabel3=Sélectionnez le dossier d'installation d'OBS Studio (celui qui contient "bin" et "obs-plugins").
german.SelectDirLabel3=Wählen Sie den Installationsordner von OBS Studio (den Ordner mit "bin" und "obs-plugins").

[CustomMessages]
italian.ObsRunning=OBS Studio è aperto. Chiudilo prima di continuare, poi premi Riprova.
english.ObsRunning=OBS Studio is running. Close it before continuing, then press Retry.
italian.ObsNotFound=In questa cartella non è stato trovato OBS Studio (manca bin\64bit\obs64.exe). Continuare comunque?
english.ObsNotFound=OBS Studio was not found in this folder (bin\64bit\obs64.exe is missing). Continue anyway?
spanish.ObsRunning=OBS Studio está abierto. Ciérralo antes de continuar y pulsa Reintentar.
spanish.ObsNotFound=No se encontró OBS Studio en esta carpeta (falta bin64bitobs64.exe). ¿Continuar de todos modos?
french.ObsRunning=OBS Studio est ouvert. Fermez-le avant de continuer, puis cliquez sur Réessayer.
french.ObsNotFound=OBS Studio est introuvable dans ce dossier (bin64bitobs64.exe manquant). Continuer quand même ?
german.ObsRunning=OBS Studio läuft. Schließen Sie es, bevor Sie fortfahren, und klicken Sie dann auf Wiederholen.
german.ObsNotFound=OBS Studio wurde in diesem Ordner nicht gefunden (bin64bitobs64.exe fehlt). Trotzdem fortfahren?

[Code]
function GetObsDir(Param: String): String;
var
  S: String;
begin
  Result := ExpandConstant('{commonpf64}\obs-studio');
  if RegQueryStringValue(HKLM64, 'SOFTWARE\OBS Studio', '', S) and (S <> '') then
    Result := S
  else if RegQueryStringValue(HKLM32, 'SOFTWARE\OBS Studio', '', S) and (S <> '') then
    Result := S;
end;

function IsObsRunning: Boolean;
var
  Wmi, Q: Variant;
begin
  Result := False;
  try
    Wmi := CreateOleObject('WbemScripting.SWbemLocator');
    Q := Wmi.ConnectServer('.', 'root\CIMV2').ExecQuery('SELECT * FROM Win32_Process WHERE Name = ''obs64.exe''');
    Result := (not VarIsNull(Q)) and (Q.Count > 0);
  except
    Result := False;
  end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = wpSelectDir then
  begin
    if not FileExists(AddBackslash(WizardDirValue) + 'bin\64bit\obs64.exe') then
      Result := MsgBox(CustomMessage('ObsNotFound'), mbConfirmation, MB_YESNO) = IDYES;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  while IsObsRunning do
  begin
    if MsgBox(CustomMessage('ObsRunning'), mbError, MB_RETRYCANCEL) = IDCANCEL then
    begin
      Result := CustomMessage('ObsRunning');
      Exit;
    end;
  end;
end;
