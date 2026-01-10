@echo off

if exist "%localappdata%/w64devkit/bin" set PATH=%localappdata%/w64devkit/bin;%PATH%


if "%~1"=="" (
	echo Drag a file in this batch file to convert it to a cpp object
	pause
	exit
)

if not "%~1"=="" (
	echo Converting file...
	ld -r -b binary %~nx1 -o %~n1.o
	pause
	exit
)
