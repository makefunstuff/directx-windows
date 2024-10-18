#!/bin/bash

CL="$HOME/code/msvc-wine/bin/x64/cl.exe" -Zi -FC
CLANG=clang -target x86_64-pc-windows-gnu

pushd build

$CLANG ../src/win32_platform.c user32.lib libcmt.lib d3d11.lib d3dcompiler.lib dxguid.lib

popd
