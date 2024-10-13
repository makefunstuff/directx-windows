@echo off
setlocal enabledelayedexpansion

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Check if shaders directory exists and contains .hlsl files
if exist res\shaders\*.hlsl (
    echo Compiling shaders...
    
    REM Compile shaders
    for %%F in (res\shaders\*.hlsl) do (
        echo Compiling shader: %%F
        set "filename=%%~nF"
        set "extension=%%~xF"
        
        REM Determine shader type based on filename
        if "!filename:~-2!"=="vs" (
            fxc /T vs_5_0 /Fo res\shaders\!filename!.cso res\shaders\%%F
        ) else if "!filename:~-2!"=="ps" (
            fxc /T ps_5_0 /Fo res\shaders\!filename!.cso res\shaders\%%F
        ) else if "!filename:~-2!"=="gs" (
            fxc /T gs_5_0 /Fo res\shaders\!filename!.cso res\shaders\%%F
        ) else if "!filename:~-2!"=="cs" (
            fxc /T cs_5_0 /Fo res\shaders\!filename!.cso res\shaders\%%F
        ) else (
            echo Unknown shader type for %%F, skipping...
        )
    )
) else (
    echo No shaders found in res\shaders directory. Skipping shader compilation.
)

REM Build the main program
pushd build
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cl -Zi -FC ..\src\win32_platform.c user32.lib libcmt.lib d3d11.lib d3dcompiler.lib dxguid.lib
popd
