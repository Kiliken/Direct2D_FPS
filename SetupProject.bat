@echo off

:: Only compatible with SDL3 ex. 3.(2.26) <-- valid sdlVersion
set sdlVersion=2.26
set buildAll=1

if exist "%cd%/Assets" set buildAll=0
if exist "%cd%/SDL.zip" goto ExitProgram


:SetupProject
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0InstallSDL.ps1" -Command %buildAll% "%sdlVersion%"
echo Project Setup completed...
pause
exit

:ExitProgram
echo Project is already installed...
pause
exit
