@echo off

if exist "%localappdata%/w64cmake/bin"	set PATH=%localappdata%/w64cmake/bin;%PATH%
if exist "%programfiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" call "%programfiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist "%programfiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" call "%programfiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%cd%/build" md build


cmake -S %cd% -B %cd%\build -G "Visual Studio 17 2022" -A x64

cmake --build %cd%/build --config Release


if %errorlevel% neq 0 (
		color 0c
		echo Build failed!
) else (
		color 0a
		echo Build succeeded. Launching game...
		cd build\Release
		App.exe
)
