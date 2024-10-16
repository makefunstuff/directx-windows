@echo off
setlocal enabledelayedexpansion

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Build the main program
pushd build
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cl -W0 -Zi -FC ..\src\win32_platform.cpp user32.lib libcmt.lib d3d11.lib d3dcompiler.lib dxguid.lib

popd
