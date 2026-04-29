param(
  [Parameter(Position = 0)]
  [string]$Uri,

  [int]$ReportId = 0
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$ConfigPath = Join-Path $ScriptRoot "LowCrashOpener.config.ps1"
Add-Type -AssemblyName System.Web

if (!(Test-Path $ConfigPath)) {
  throw "Missing config: $ConfigPath. Copy LowCrashOpener.config.ps1.example and fill in the private values."
}

. $ConfigPath

function Get-ReportIdFromUri {
  param([string]$Value)

  if ([string]::IsNullOrWhiteSpace($Value)) {
    return 0
  }

  try {
    $parsed = [Uri]$Value
    $query = [System.Web.HttpUtility]::ParseQueryString($parsed.Query)
    $idValue = $query.Get("id")
    if (![string]::IsNullOrWhiteSpace($idValue)) {
      return [int]$idValue
    }
  } catch {
  }

  if ($Value -match "id=(\d+)") {
    return [int]$Matches[1]
  }

  if ($Value -match "^\d+$") {
    return [int]$Value
  }

  return 0
}

function Find-Devenv {
  param([string]$ConfiguredPath)

  if (![string]::IsNullOrWhiteSpace($ConfiguredPath) -and
      (Test-Path $ConfiguredPath)) {
    return (Resolve-Path $ConfiguredPath).Path
  }

  $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path $vswhere) {
    $path = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "Common7\IDE\devenv.exe" | Select-Object -First 1
    if (![string]::IsNullOrWhiteSpace($path) -and (Test-Path $path)) {
      return $path
    }
  }

  $candidates = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\Common7\IDE\devenv.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\devenv.exe"
  )

  foreach ($candidate in $candidates) {
    if (Test-Path $candidate) {
      return $candidate
    }
  }

  throw "Could not find devenv.exe. Set `$DevenvPath in LowCrashOpener.config.ps1."
}

function Get-BasicAuthHeader {
  param([string]$User, [string]$PlainPassword)

  if ([string]::IsNullOrWhiteSpace($User)) {
    return @{}
  }

  $bytes = [Text.Encoding]::ASCII.GetBytes("${User}:${PlainPassword}")
  return @{ Authorization = "Basic " + [Convert]::ToBase64String($bytes) }
}

if ($ReportId -le 0) {
  $ReportId = Get-ReportIdFromUri $Uri
}

if ($ReportId -le 0) {
  throw "Missing crash report id. Expected lowengine-crash://open?id=123 or -ReportId 123."
}

if ([string]::IsNullOrWhiteSpace($ServerBaseUrl)) {
  throw "ServerBaseUrl is empty in LowCrashOpener.config.ps1."
}

if ([string]::IsNullOrWhiteSpace($DownloadRoot)) {
  $DownloadRoot = Join-Path $env:LOCALAPPDATA "LowEngine\CrashReports"
}

New-Item -ItemType Directory -Force -Path $DownloadRoot | Out-Null

$reportRoot = Join-Path $DownloadRoot ("report-" + $ReportId)
$zipPath = Join-Path $DownloadRoot ("report-" + $ReportId + ".zip")
$downloadUrl = $ServerBaseUrl.TrimEnd("/") + "/download.php?id=" + $ReportId

Write-Host "Downloading crash report #$ReportId"
Invoke-WebRequest `
  -Uri $downloadUrl `
  -OutFile $zipPath `
  -Headers (Get-BasicAuthHeader $Username $Password) `
  -UseBasicParsing

if (Test-Path $reportRoot) {
  Remove-Item -LiteralPath $reportRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $reportRoot | Out-Null
Expand-Archive -Path $zipPath -DestinationPath $reportRoot -Force

$dump = Get-ChildItem -Path $reportRoot -Recurse -Filter "minidump.dmp" -File | Select-Object -First 1
if (!$dump) {
  throw "Downloaded report does not contain minidump.dmp."
}

$manifest = Get-ChildItem -Path $reportRoot -Recurse -Filter "release_manifest.json" -File | Select-Object -First 1
$releaseId = ""
if ($manifest) {
  $manifestJson = Get-Content -Path $manifest.FullName -Raw | ConvertFrom-Json
  $releaseId = [string]$manifestJson.release_id
}

$symbolPathParts = @()
if (![string]::IsNullOrWhiteSpace($SymbolRoot)) {
  if (![string]::IsNullOrWhiteSpace($releaseId)) {
    $releaseSymbols = Join-Path $SymbolRoot $releaseId
    if (Test-Path $releaseSymbols) {
      $symbolPathParts += $releaseSymbols
    }
  }

  if (Test-Path $SymbolRoot) {
    $symbolPathParts += $SymbolRoot
  }
}

$symbolCache = Join-Path $DownloadRoot "symbol_cache"
New-Item -ItemType Directory -Force -Path $symbolCache | Out-Null
$symbolPathParts += "srv*$symbolCache*https://msdl.microsoft.com/download/symbols"

$env:_NT_SYMBOL_PATH = ($symbolPathParts | Select-Object -Unique) -join ";"

$devenv = Find-Devenv $DevenvPath

Write-Host "Opening dump in Visual Studio"
Write-Host "Dump: $($dump.FullName)"
Write-Host "Symbols: $env:_NT_SYMBOL_PATH"

$visualStudioArguments = @('"' + $dump.FullName + '"')
if ($AutoStartVisualStudioDebugging -and
    ![string]::IsNullOrWhiteSpace($VisualStudioAutoDebugCommand)) {
  $visualStudioArguments += "/Command"
  $visualStudioArguments += $VisualStudioAutoDebugCommand
  Write-Host "Auto-start command: $VisualStudioAutoDebugCommand"
}

$visualStudioProcess = Start-Process `
  -FilePath $devenv `
  -ArgumentList $visualStudioArguments `
  -PassThru

if ($CleanupAfterVisualStudioCloses) {
  Write-Host "Waiting for Visual Studio to close before cleaning local crash files..."
  try {
    $visualStudioProcess.WaitForExit()
  } catch {
    Write-Warning "Could not wait for Visual Studio. Local files were left in: $reportRoot"
    exit 0
  }

  Write-Host "Cleaning local crash files..."
  Remove-Item -LiteralPath $reportRoot -Recurse -Force -ErrorAction SilentlyContinue
  Remove-Item -LiteralPath $zipPath -Force -ErrorAction SilentlyContinue
}
