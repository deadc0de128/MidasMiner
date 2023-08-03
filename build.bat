@echo off
setlocal

set PRESET=x64-release

if "%1" NEQ "" set PRESET=%1

set BUILD_DIR=windows-build\%PRESET%

echo Build type = %PRESET%

rmdir/S/Q %BUILD_DIR%

set SDL2_DIR=..\..\SDL2-2.28.2
set SDL2_IMAGE_DIR=..\..\SDL2_image-2.6.3

cmake -S . --preset %PRESET% 
cmake --build %BUILD_DIR% --verbose

endlocal
