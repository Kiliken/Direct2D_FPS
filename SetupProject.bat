@echo off

:FileCheck
if exist "%cd%/dep" goto ExitProgram
if not exist "%localappdata%/w64cmake" goto InstallCMake


:SetupProject
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup/InstallDep.ps1"
echo Project Setup completed...
pause
exit


:InstallCMake
echo Installing Gcc...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup/InstallCMake.ps1"
echo Waiting for CMake to finish installing...
pause
goto FileCheck

:ExitProgram
echo Project is already installed...
pause
exit