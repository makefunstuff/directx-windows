@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Build the main program
pushd build


cl -W0 -Zi -FC ..\src\win32_platform.cpp user32.lib libcmt.lib d3d11.lib d3dcompiler.lib dxguid.lib

popd
