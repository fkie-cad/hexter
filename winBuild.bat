@echo off

set target=hexter
set ct=Application
set /a bitness=64
set platform=x64
set mode=Release
set /a rt=0
set pdb=no
set buildTools="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools"

set prog_name=%~n0
set user_dir="%~dp0"
set verbose=1



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
    IF "%~1"=="/b" (
        SET /a bitness=%~2
        SHIFT
        goto reParseParams
    )
    IF "%~1"=="/m" (
        SET mode=%~2
        SHIFT
        goto reParseParams
    )
    IF "%~1"=="/bt" (
        SET buildTools=%~2
        SHIFT
        goto reParseParams
    )
    IF /i "%~1"=="/rt" (
        SET /a rt=1
        goto reParseParams
    )
    IF /i "%~1"=="/pdb" (
        SET /a pdb=yes
        goto reParseParams
    )
    
    :reParseParams
        SHIFT
        if [%1]==[] goto main

GOTO :ParseParams


:main

set build_dir=build\%bitness%
if /i [%mode%]==[debug] set build_dir=build\debug\%bitness%

set valid=0
if [%bitness%] == [32] (
    set platform=x86
    set valid=1
) else (
    if [%bitness%] == [64] (
        set platform=x64
        set valid=1
    )
)
if [valid] == [0] (
    goto help
)

set valid=0
if /i [%target%] == [hexter] (
    set ct=Application
    set valid=1
) else (
    if /i [%target%] == [hexter_lib] (
        set ct=DynamicLibrary
        set valid=1
    )
)
if [valid] == [0] (
    goto help
)

set rtlib=No
set valid=0
if /i [%mode%] == [debug] (
    if [%rt%] == [1] (
        set rtlib=Debug
    )
    set valid=1
) else (
    if /i [%mode%] == [release] (
        if [%rt%] == [1] (
            set rtlib=Release
        )
        set valid=1
    )
)
if [valid] == [0] (
    goto help
)

if [%verbose%] == [1] (
    echo target=%target%
    echo ConfigurationType=%ct%
    echo bitness=%bitness%
    echo platform=%platform%
    echo mode=%mode%
    echo build_dir=%build_dir%
    echo buildTools=%buildTools%
)

rem vcvarsall.bat [architecture] [platform_type] [winsdk_version] [ -vcvars_ver= vcversion]
rem architecture = x86, x86_x64, ... 

set vcvars="%buildTools:~1,-1%\VC\Auxiliary\Build\vcvars%bitness%.bat"

:build
    REM cmd /k "mkdir %build_dir% & %vcvars% & %cmake% -S . -B %build_dir% -DCMAKE_BUILD_TYPE=%mode% -DMT=%rt% -DPDB=%pdb% -G "NMake Makefiles" & %cmake% --build %build_dir% --config %mode% --target %target% & exit"
    echo cmd /k "mkdir %build_dir% & %vcvars% & msbuild Hexter.vcxproj /p:Platform=%platform% /p:Configuration=%mode% /p:RuntimeLib=%rtlib% /p:PDB=%pdb% /p:ConfigurationType=%ct%  & exit"
    cmd /k "mkdir %build_dir% & %vcvars% & msbuild Hexter.vcxproj /p:Platform=%platform% /p:Configuration=%mode% /p:RuntimeLib=%rtlib% /p:PDB=%pdb% /p:ConfigurationType=%ct%  & exit"

    if /i [%mode%]==[release] (
        certutil -hashfile %build_dir%\%target%.exe sha256 | find /i /v "sha256" | find /i /v "certutil" > %build_dir%\%target%.sha256
    )

    exit /B 0

:usage
    @echo Usage: %prog_name% [/t %target%^|%target%_lib] [/b 32^|64] [/m Debug^|Release] [/bt C:\Build\Tools\] [/rt] [/pdb] [/h]
    @echo Default: %prog_name% [/t %target% /b %bitness% /m %mode% /bt %buildTools%]
    exit /B 0

:help
    call :usage
	echo.
	echo Options:
    @echo /t The target name to build. Default: hexter.
    @echo /b The target bitness. Default: 64.
    @echo /m The mode (Debug^|Release) to build in. Default: Release.
    @echo /bt Custom path to Microsoft Visual Studio BuildTools
    @echo /rt Statically include LIBCMT.lib. Increases file size but may be needed if a "VCRUNTIMExxx.dll not found Error" occurs on the target system.
    @echo /pdb Include pdb symbols into release build. Default in debug mode. 
    exit /B 0
