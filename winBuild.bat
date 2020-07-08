@echo off

set target=hexter
set bitness=64
set mode=Release
set buildTools="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools"
set cmake_path="C:\Program Files\cmake\bin\cmake.exe"
set cmake=cmake

set prog_name=%~n0
set user_dir="%~dp0"
set verbose=1


WHERE %cmake% >nul 2>nul
IF %ERRORLEVEL% NEQ 0 set cmake=%cmake_path%


GOTO :ParseParams

:ParseParams

    REM IF "%~1"=="" GOTO Main
    if [%1]==[/?] goto help
    if [%1]==[/h] goto help
    if [%1]==[/help] goto help

    IF "%~1"=="/t" (
        SET target=%2
        SHIFT
        goto reParseParams
    )
    IF "%~1"=="/target" (
        SET target=%2
        SHIFT
        goto reParseParams
    )
    IF "%~1"=="/b" (
        SET bitness=%~2
        SHIFT
        goto reParseParams
    )
    IF "%~1"=="/bitness" (
        SET bitness=%~2
        SHIFT
        goto reParseParams
    )
    IF "%~1"=="/m" (
        SET mode=%~2
        SHIFT
        goto reParseParams
    )
    IF "%~1"=="/mode" (
        SET mode=%~2
        SHIFT
        goto reParseParams
    )
    IF "%~1"=="/bt" (
        SET buildTools=%~2
        SHIFT
        goto reParseParams
    )
    IF "%~1"=="/buildtools" (
        SET buildTools=%~2
        SHIFT
        goto reParseParams
    )
    
    :reParseParams
        SHIFT
        if [%1]==[] goto main

GOTO :ParseParams


:main

set build_dir=build\%bitness%
if /i [%mode%]==[debug] set build_dir=build\debug\%bitness%

echo target=%target%
echo bitness=%bitness%
echo mode=%mode%
echo build_dir=%build_dir%
echo buildTools=%buildTools%

rem vcvarsall.bat [architecture] [platform_type] [winsdk_version] [ -vcvars_ver= vcversion]
rem architecture = x86, x86_x64, ... 

set vcvars="%buildTools:~1,-1%\VC\Auxiliary\Build\vcvars%bitness%.bat"

:build
    cmd /k "mkdir %build_dir% & %vcvars% & %cmake% -S . -B %build_dir% -DCMAKE_BUILD_TYPE=%mode% -G "NMake Makefiles" & %cmake% --build %build_dir% --config %mode% --target %target% & exit"
    exit /B 0

:usage
    @echo Usage: %prog_name% [/t %target%^|%target%_shared] [/b 32^|64] [/m Debug^|Release] [/bt C:\Build\Tools\] [/h]
    @echo Default: %prog_name% [/t %target% /b %bitness% /m %mode% /bt %buildTools%]
    exit /B 0

:help
    call :usage
    @echo /t^|/target The target name to build. Default: hexter.
    @echo /b^|/bitness The target bitness. Default: 64.
    @echo /m^|/mode The mode (Debug^|Release) to build in. Default: Release.
    @echo /bt^|/buildtools Custom path to Microsoft Visual Studio BuildTools
    exit /B 0
