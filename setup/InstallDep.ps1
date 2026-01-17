

$ProjectRoot = "$($PSScriptRoot)\.."

New-Item -Path "$($ProjectRoot)\build\Release" -ItemType Directory
New-Item -Path "$($ProjectRoot)\dep" -ItemType Directory
New-Item -Path "$($ProjectRoot)\dep\include" -ItemType Directory
New-Item -Path "$($ProjectRoot)\dep\lib" -ItemType Directory

$SdlDownloadUrl = "https://github.com/libsdl-org/SDL/releases/download/release-3.2.26/SDL3-devel-3.2.26-VC.zip"
$imguiDownloadUrl = "https://github.com/ocornut/imgui/archive/refs/tags/v1.92.5.zip"

Add-Type -AssemblyName System.IO.Compression.FileSystem

Write-Output "[sdl3-vsbt-setup] Downloading SDL3-devel-3.2.26-VC..."
curl.exe -L "$SdlDownloadUrl" -o "$($PSScriptRoot)\SDL.zip" --progress-bar
Write-Output "[openGL-portable-setup] Downloading ImGUI..."
curl.exe -L "$imguiDownloadUrl" -o "$($PSScriptRoot)\imgui.zip" --progress-bar

Write-Output "[sdl3-vsbt-setup] Extracting SDL3-devel-3.2.26-VC..."
[System.IO.Compression.ZipFile]::ExtractToDirectory("$($PSScriptRoot)\SDL.zip", $PSScriptRoot)
Write-Output "[openGL-portable-setup] Extracting ImGUI..."
[System.IO.Compression.ZipFile]::ExtractToDirectory("$($PSScriptRoot)\imgui.zip", "$($ProjectRoot)\dep\include")

Copy-Item -Path "$($PSScriptRoot)\SDL3-3.2.26\include\SDL3" -Destination "$($ProjectRoot)\dep\include" -Recurse
Copy-Item -Path "$($PSScriptRoot)\SDL3-3.2.26\lib\x64\SDL3.lib" -Destination "$($ProjectRoot)\dep\lib"
Copy-Item -Path "$($PSScriptRoot)\SDL3-3.2.26\lib\x64\SDL3.dll" -Destination "$($ProjectRoot)\build\Release"

Rename-Item -Path "$($ProjectRoot)\dep\include\imgui-1.92.5" -NewName "imgui"
Copy-Item -Path "$($ProjectRoot)\dep\include\imgui\backends\imgui_impl_dx11.cpp" -Destination "$($ProjectRoot)\dep\include\imgui"
Copy-Item -Path "$($ProjectRoot)\dep\include\imgui\backends\imgui_impl_dx11.h" -Destination "$($ProjectRoot)\dep\include\imgui"
Copy-Item -Path "$($ProjectRoot)\dep\include\imgui\backends\imgui_impl_sdl3.cpp" -Destination "$($ProjectRoot)\dep\include\imgui"
Copy-Item -Path "$($ProjectRoot)\dep\include\imgui\backends\imgui_impl_sdl3.h" -Destination "$($ProjectRoot)\dep\include\imgui"



return 0
