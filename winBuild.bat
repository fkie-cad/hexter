@echo off

set target=hexter
set bitness=64
set mode=Release
set buildTools="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools"

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
if [%mode%]==[Debug] set build_dir=build\debug\%bitness%
if [%mode%]==[debug] set build_dir=build\debug\%bitness%

echo target=%target%
echo bitness=%bitness%
echo mode=%mode%
echo build_dir=%build_dir%
echo buildTools=%buildTools%

rem vcvarsall.bat [architecture] [platform_type] [winsdk_version] [ -vcvars_ver= vcversion]
rem architecture = x86, x86_x64, ... 

set vcvars="%buildTools:~1,-1%\VC\Auxiliary\Build\vcvars%bitness%.bat"

:build
    cmd /k "mkdir %build_dir% & %vcvars% & cmake -S . -B %build_dir% -DCMAKE_BUILD_TYPE=%mode% -G "NMake Makefiles" & cmake --build %build_dir% --config %mode% --target %target% & exit"
    exit /B 0

:usage
    @echo Usage: %0 [/t %target%^|%target%_shared] [/b 32^|64] [/m Debug^|Release] [/bt C:\Build\Tools\] [/h]
    @echo Default: %0 [/t %target% /b %bitness% /m %mode% /bt %buildTools%]
    exit /B 0

:help
    call :usage
    @echo /t^|/target The target name to build.
    @echo /b^|/bitness The target bitness. Default: 32.
    @echo /m^|/mode The mode (Debug^|Release) to build in. Default: Debug.
    @echo /bt^|/buildtools Custom path to Microsoft Visual Studio BuildTools
    exit /B 0
