param([string]$InstallDir = "$env:LOCALAPPDATA\Programs\Basilisk Battery")
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $repo
$version = (Get-Content VERSION -Raw).Trim()
$setupPath = (Resolve-Path -LiteralPath "artifacts/BasiliskBattery-Setup-$version.exe").Path
$appPath = Join-Path $InstallDir 'BasiliskBattery.exe'
$runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$valueName = 'BasiliskBattery'
if (-not (Test-Path -LiteralPath $runKey)) { New-Item -Path $runKey | Out-Null }
function Get-StartupSnapshot {
    $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('Software\Microsoft\Windows\CurrentVersion\Run')
    try {
        $values = [ordered]@{}
        foreach ($name in ($key.GetValueNames() | Sort-Object)) {
            $values[$name] = @{ Kind = $key.GetValueKind($name).ToString(); Value = $key.GetValue($name, $null, [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames) }
        }
        return ($values | ConvertTo-Json -Depth 5 -Compress)
    } finally { $key.Close() }
}
$unrelatedBefore = Get-StartupSnapshot
$saved = (Get-ItemProperty -LiteralPath $runKey).$valueName
if ($null -ne $saved) { throw 'Teste do instalador requer preferência de startup ausente para não substituir uma escolha existente.' }
$startupCommand = '"' + $appPath + '"'
try {
    $setupArgs = @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', ('/DIR="' + $InstallDir + '"'), '/LOG=artifacts/installer-smoke.log')
    $first = Start-Process -FilePath $setupPath -ArgumentList $setupArgs -PassThru -Wait -WindowStyle Hidden
    if ($first.ExitCode -ne 0) { throw "Primeira instalação falhou: $($first.ExitCode)" }
    $running = Start-Process -FilePath $appPath -PassThru -WindowStyle Hidden
    Start-Sleep -Milliseconds 600
    $running.Refresh()
    if ($running.HasExited) { throw 'Processo instalado encerrou inesperadamente.' }
    Set-ItemProperty -LiteralPath $runKey -Name $valueName -Value $startupCommand
    $upgrade = Start-Process -FilePath $setupPath -ArgumentList $setupArgs -PassThru -Wait -WindowStyle Hidden
    if ($upgrade.ExitCode -ne 0) { throw "Upgrade com app aberto falhou: $($upgrade.ExitCode)" }
    if (-not $running.WaitForExit(4000)) { throw 'Instância anterior permaneceu aberta.' }
    if ((Get-ItemProperty -LiteralPath $runKey).$valueName -ne $startupCommand) { throw 'Upgrade perdeu a preferência de startup.' }
    if ((Get-Item -LiteralPath $appPath).VersionInfo.ProductVersion -ne $version) { throw 'Versão instalada divergente.' }
    if ((Get-FileHash -LiteralPath $appPath).Hash -ne (Get-FileHash build/BasiliskBattery.exe).Hash) { throw 'Executável instalado divergente.' }
    Write-Host "Installer smoke passed: $version, running app closed, startup preference preserved."
} finally {
    Remove-ItemProperty -LiteralPath $runKey -Name $valueName -ErrorAction SilentlyContinue
    if ((Get-StartupSnapshot) -cne $unrelatedBefore) { throw 'Teste alterou entradas de startup não relacionadas.' }
}
