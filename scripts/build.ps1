param([string]$Toolchain = '', [switch]$Installer)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $repo
$version = (Get-Content -LiteralPath VERSION -Raw).Trim()
if ($version -notmatch '^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$') { throw 'VERSION deve conter MAJOR.MINOR.PATCH.' }
$parts = $version.Split('.')
if (($parts | Where-Object { [long]$_ -gt 65535 }).Count) { throw 'Componentes da versão devem ser <= 65535.' }
$changelog = Get-Content -LiteralPath CHANGELOG.md -Raw
$match = [regex]::Match($changelog, '(?ms)^## \[' + [regex]::Escape($version) + '\][^\r\n]*\r?\n(.*?)(?=^## |\z)')
if (-not $match.Success -or -not $match.Groups[1].Value.Trim()) { throw 'Faltam notas da versão atual em CHANGELOG.md.' }
New-Item -ItemType Directory -Force build,artifacts | Out-Null
$utf8 = New-Object System.Text.UTF8Encoding($false)
[IO.File]::WriteAllText((Join-Path $repo 'build/version.h'), "#pragma once`n#define APP_VERSION `"$version`"`n#define APP_VERSION_W L`"$version`"`n", $utf8)
$icon = (Join-Path $repo 'assets/app.ico').Replace('\','/')
$manifest = (Join-Path $repo 'src/app.manifest').Replace('\','/')
$rc = @"
#include <windows.h>
1 ICON "$icon"
1 RT_MANIFEST "$manifest"
1 VERSIONINFO
 FILEVERSION $($parts -join ','),0
 PRODUCTVERSION $($parts -join ','),0
 FILEFLAGSMASK 0x3fL
 FILEFLAGS 0
 FILEOS VOS_NT_WINDOWS32
 FILETYPE VFT_APP
BEGIN
 BLOCK "StringFileInfo"
 BEGIN
  BLOCK "040904b0"
  BEGIN
   VALUE "CompanyName", "luingry\0"
   VALUE "FileDescription", "Basilisk Battery\0"
   VALUE "FileVersion", "$version\0"
   VALUE "InternalName", "BasiliskBattery\0"
   VALUE "OriginalFilename", "BasiliskBattery.exe\0"
   VALUE "ProductName", "Basilisk Battery\0"
   VALUE "ProductVersion", "$version\0"
  END
 END
 BLOCK "VarFileInfo"
 BEGIN
  VALUE "Translation", 0x409, 1200
 END
END
"@
[IO.File]::WriteAllText((Join-Path $repo 'build/app.rc'), $rc, $utf8)
[IO.File]::WriteAllText((Join-Path $repo 'artifacts/release-notes.md'), $match.Groups[1].Value.Trim() + "`n", $utf8)
function Assert-Exit([string]$step) { if ($LASTEXITCODE -ne 0) { throw "$step falhou: $LASTEXITCODE" } }
if ($Toolchain) {
    $gpp = Join-Path $Toolchain 'g++.exe'
    $windres = Join-Path $Toolchain 'windres.exe'
    if (-not (Test-Path -LiteralPath $gpp)) { throw "Compilador não encontrado: $gpp" }
    & $gpp -std=c++17 -O2 -Wall -Wextra -Werror -Isrc tests/protocol_tests.cpp -o build/protocol_tests.exe
    Assert-Exit 'Compilação dos testes'
    & $gpp -std=c++17 -O2 -Wall -Wextra -Werror -Isrc tests/startup_tests.cpp src/startup.cpp -ladvapi32 -o build/startup_tests.exe
    Assert-Exit 'Compilação dos testes Windows'
    & $windres -i build/app.rc -o build/app.o
    Assert-Exit 'Recursos'
    & $gpp -std=c++17 -Os -Wall -Wextra -Werror -Wno-cast-function-type -municode -mwindows -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -Isrc -Ibuild src/main.cpp src/device.cpp src/icon.cpp src/startup.cpp build/app.o -static -s '-Wl,--dynamicbase,--nxcompat,--high-entropy-va' -lsetupapi -lhid -lshell32 -ladvapi32 -lgdi32 -luser32 -o build/BasiliskBattery.exe
    Assert-Exit 'Compilação do app'
} else {
    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw 'Use um Developer PowerShell do Visual Studio ou -Toolchain <pasta bin do w64devkit>.' }
    & cl.exe /nologo /std:c++17 /O2 /W4 /WX /MT /EHsc /Isrc tests/protocol_tests.cpp /Febuild/protocol_tests.exe /Fobuild/protocol_tests.obj
    Assert-Exit 'Compilação dos testes'
    & cl.exe /nologo /std:c++17 /O2 /W4 /WX /MT /EHsc /Isrc tests/startup_tests.cpp src/startup.cpp /Febuild/startup_tests.exe /Fobuild/ /link advapi32.lib
    Assert-Exit 'Compilação dos testes Windows'
    & rc.exe /nologo /fobuild/app.res build/app.rc
    Assert-Exit 'Recursos'
    & cl.exe /nologo /std:c++17 /O1 /W4 /WX /MT /EHsc /utf-8 /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /Isrc /Ibuild src/main.cpp src/device.cpp src/icon.cpp src/startup.cpp /Fobuild/ /Febuild/BasiliskBattery.exe /link build/app.res /SUBSYSTEM:WINDOWS /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA setupapi.lib hid.lib shell32.lib advapi32.lib gdi32.lib user32.lib
    Assert-Exit 'Compilação do app'
}
& ./build/protocol_tests.exe
Assert-Exit 'Testes de protocolo'
& ./build/startup_tests.exe
Assert-Exit 'Testes de integração Windows'
$exeVersion = (Get-Item -LiteralPath build/BasiliskBattery.exe).VersionInfo.ProductVersion
if ($exeVersion -ne $version) { throw "Versão compilada divergente: $exeVersion" }
Copy-Item -LiteralPath build/BasiliskBattery.exe -Destination "artifacts/BasiliskBattery-$version.exe"
Write-Host "Basilisk Battery $version compilado: $((Get-Item build/BasiliskBattery.exe).Length) bytes."
if ($Installer) {
    $candidates = @("$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe", "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "${env:ProgramFiles}\Inno Setup 6\ISCC.exe")
    $iscc = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
    if (-not $iscc) { $iscc = (Get-Command iscc.exe -ErrorAction SilentlyContinue).Source }
    if (-not $iscc) { throw 'Inno Setup 6 não encontrado.' }
    & $iscc "/DAppVersion=$version" installer/BasiliskBattery.iss
    Assert-Exit 'Instalador'
    $installerPath = Join-Path $repo "artifacts/BasiliskBattery-Setup-$version.exe"
    $hash = (Get-FileHash -LiteralPath $installerPath -Algorithm SHA256).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText((Join-Path $repo "artifacts/BasiliskBattery-Setup-$version.sha256"), "$hash  BasiliskBattery-Setup-$version.exe`n", $utf8)
}
