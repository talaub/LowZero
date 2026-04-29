$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$OpenScript = Join-Path $ScriptRoot "Open-LowCrash.ps1"

if (!(Test-Path $OpenScript)) {
  throw "Missing opener script: $OpenScript"
}

$ProtocolRoot = "HKCU:\Software\Classes\lowengine-crash"
$CommandKey = Join-Path $ProtocolRoot "shell\open\command"

New-Item -Path $ProtocolRoot -Force | Out-Null
Set-Item -Path $ProtocolRoot -Value "URL:LowEngine Crash Report"
New-ItemProperty -Path $ProtocolRoot -Name "URL Protocol" -Value "" -PropertyType String -Force | Out-Null

New-Item -Path $CommandKey -Force | Out-Null
$command = 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "' + $OpenScript + '" "%1"'
Set-Item -Path $CommandKey -Value $command

Write-Host "Registered lowengine-crash:// links for the current Windows user."
Write-Host "Command: $command"
