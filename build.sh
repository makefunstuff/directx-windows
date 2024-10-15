#!/bin/bash

CL="$HOME/code/msvc-wine/bin/x64/cl.exe"

pushd build

$CL -Zi -FC ../src/win32_platform.c user32.lib libcmt.lib d3d11.lib d3dcompiler.lib dxguid.lib

popd
