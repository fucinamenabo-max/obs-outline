# Installa obs-outline nell'OBS locale (Steam). Chiudere OBS prima.
param([string]$ObsDir = 'C:\Program Files (x86)\Steam\steamapps\common\OBS Studio')
$src = Join-Path $PSScriptRoot 'build_vs17\rundir\Release'
if (Get-Process obs64 -ErrorAction SilentlyContinue) { throw 'OBS e'' aperto: chiudilo e rilancia.' }
$dll = Join-Path $ObsDir 'obs-plugins\64bit'
$data = Join-Path $ObsDir 'data\obs-plugins\obs-outline'
New-Item -ItemType Directory -Force $dll, $data | Out-Null
Copy-Item (Join-Path $src 'obs-outline.dll') $dll -Force
Copy-Item (Join-Path $PSScriptRoot 'data\*') $data -Recurse -Force
Write-Host "Installato in $ObsDir"
