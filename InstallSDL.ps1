
$BuildProj = $Args[1]
$SdlVersion = $Args[2]

$SdlDownloadUrl = "https://github.com/libsdl-org/SDL/releases/download/release-3.$SdlVersion/SDL3-devel-3.$SdlVersion-VC.zip"

$wc = New-Object Net.WebClient
Add-Type -AssemblyName System.IO.Compression.FileSystem

Write-Output "[sdl3-vsbt-setup] Downloading SDL3-devel-3.$SdlVersion-VC..."
$wc.DownloadFile($SdlDownloadUrl, "$($PSScriptRoot)\SDL.zip")

Write-Output "[sdl3-vsbt-setup] Extracting SDL3-devel-3.$SdlVersion-VC..."
[System.IO.Compression.ZipFile]::ExtractToDirectory("$($PSScriptRoot)\SDL.zip", $PSScriptRoot)

New-Item -Path "$($PSScriptRoot)\build" -ItemType Directory
Copy-Item -Path "$($PSScriptRoot)\SDL3-3.$SdlVersion\lib\x64\SDL3.dll" -Destination "$($PSScriptRoot)\build"

if ($BuildProj -eq 1) {
	Write-Output "[sdl3-vsbt-setup] No avible project detected"
	Write-Output "[sdl3-vsbt-setup] Setting up template..."
	
	New-Item -Path "$($PSScriptRoot)\Assets" -ItemType Directory
	Add-Content -Path "$($PSScriptRoot)\.gitignore" -Value "/[Ss]DL3-3.$SdlVersion/"
	(Get-Content -Path "$($PSScriptRoot)\BuildGame.bat") -replace 'xVersioNx', $SdlVersion | Set-Content -Path "$($PSScriptRoot)\BuildGame.bat"

	
}

return 0
