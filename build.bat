@echo off
setlocal

set PRESET=x64-release

if "%1" NEQ "" set PRESET=%1

set BUILD_DIR=windows-build\%PRESET%

echo Build type = %PRESET%

rmdir/S/Q %BUILD_DIR%

cmake -S . --preset %PRESET% 
cmake --build %BUILD_DIR% --verbose

endlocal
