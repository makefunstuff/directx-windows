@echo off

if not exist build mkdir build
pushd build

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cl -Zi -FC ..\src\win32_platform.c user32.lib libcmt.lib d3d11.lib d3dcompiler.lib dxguid.lib
popd

