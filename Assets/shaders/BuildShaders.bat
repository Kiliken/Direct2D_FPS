@echo off

if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.26100.0\x64\fxc.exe" set compiler="%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.26100.0\x64\fxc.exe"

SET includes=/I "%ProgramFiles(x86)%\Windows Kits\10\Include\10.0.26100.0\um"

::%compiler% /?

for %%f in (*.hlsl) do (
    echo Compiling %%f ...
    %compiler% /T lib_4_1 ^
        /D D2D_FUNCTION ^
        %includes% ^
        /Fo "%%~nf.cso" "%%f"
	
	if !errorlevel! neq 0 (
        color 0c
        echo Build failed for %%f!
        pause
    ) else (
        color 0a
        echo Build succeeded for %%f...
    )
)

pause
