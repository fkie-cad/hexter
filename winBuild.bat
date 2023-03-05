@echo off
setlocal

set my_name=%~n0
set my_dir="%~dp0"

set name=hexter

set /a app=0
set /a lib=0
set /a cln=0

set /a bitness=64
set platform=x64
set mode=Release
set /a rtl=0
set /a pdb=0
set /a ico=1
set /a verbose=0

:: adjust this path, if you're using another version or path.
set buildTools="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools"
set pts=v142
:: set pts=WindowsApplicationForDrivers10.0


:: default
if [%1]==[] goto main

GOTO :ParseParams

:ParseParams

    REM IF "%~1"=="" GOTO Main
    if [%1]==[/?] goto help
    if [%1]==[/h] goto help
    if [%1]==[/help] goto help
    
    IF /i "%~1"=="/app" (
        SET /a app=1
        goto reParseParams
    )
    IF /i "%~1"=="/lib" (
        SET /a lib=1
        goto reParseParams
    )
    IF /i "%~1"=="/cln" (
        SET /a cln=1
        goto reParseParams
    )

    IF /i "%~1"=="/b" (
        SET /a bitness=%~2
        SHIFT
        goto reParseParams
    )
    IF /i "%~1"=="/m" (
        SET mode=%~2
        SHIFT
        goto reParseParams
    )
    IF /i "%~1"=="/bt" (
        SET buildTools=%~2
        SHIFT
        goto reParseParams
    )
    IF /i "%~1"=="/pts" (
        SET pts=%~2
        SHIFT
        goto reParseParams
    )
    IF /i "%~1"=="/rtl" (
        SET /a rtl=1
        goto reParseParams
    )
    IF /i "%~1"=="/pdb" (
        SET /a pdb=1
        goto reParseParams
    )
    IF /i "%~1"=="/xi" (
        SET /a ico=0
        goto reParseParams
    )
    
    IF /i "%~1"=="/v" (
        SET /a verbose=1
        goto reParseParams
    ) ELSE (
        echo Unknown option : "%~1"
    )
    
    :reParseParams
        SHIFT
        if [%1]==[] goto main

GOTO :ParseParams


:main

    :: set platform
    set /a valid=0
    if %bitness% == 32 (
        set platform=x86
        set /a valid=1
    ) else (
        if %bitness% == 64 (
            set platform=x64
            set valid=1
        )
    )
    if %valid% == 0 (
        goto help
    )

    :: test valid targets
    set /a "valid=%app%+%lib%+%cln%"
    if %valid% == 0 (
        set /a app=1
    )
    
    :: set runtime lib
    set rtlib=No
    set /a valid=0
    if /i [%mode%] == [debug] (
        if %rtl% == 1 (
            set rtlib=Debug
        )
        set /a pdb=1
        set /a valid=1
    ) else (
        if /i [%mode%] == [release] (
            if %rtl% == 1 (
                set rtlib=Release
            )
            set /a valid=1
        )
    )
    if %valid% == 0 (
        goto help
    )

    :: verbose print
    if [%verbose%] == [1] (
        echo app=%app%
        echo lib=%lib%
        echo bitness=%bitness%
        echo platform=%platform%
        echo mode=%mode%
        echo buildTools=%buildTools%
        echo rtlib=%rtlib%
        echo pts=%pts%
        echo ico=%ico%
        echo proj=%proj%
    )

    
    :: set vcvars, if necessary
    :: pseudo nop command to prevent if else bug in :build
    set vcvars=call
    if [%VisualStudioVersion%] EQU [] (
        if not exist %buildTools% (
            echo [e] No build tools found in %buildTools%!
            echo     Please set the correct path in this script or with the /bt option.
            exit /b -1
        )
        set vcvars="%buildTools:~1,-1%\VC\Auxiliary\Build\vcvars%bitness%.bat"
    )

    :: build targets
    if %cln% == 1 (
        rmdir /s /q build
    )
    if %app% == 1 (
        call :build Hexter.vcxproj Application
    ) 
    if %lib% == 1 (
        call :build Hexter.vcxproj DynamicLibrary
    ) 

    endlocal
    exit /B 0


:build
    setlocal
        set proj=%1
        set ct=%2
        
        cmd /k "%vcvars% & msbuild Hexter.vcxproj /p:Platform=%platform% /p:PlatformToolset=%pts% /p:Configuration=%mode% /p:RuntimeLib=%rtlib% /p:PDB=%pdb% /p:ConfigurationType=%ct% /p:Icon=%ico% & exit"
        
    endlocal
    exit /B 0

:usage
    @echo Usage: %my_name% [/app] [/lib] [/b 32^|64] [/m Debug^|Release] [/rtl] [/pdb] [/pts ^<toolset^>] [/bt ^<path^>] [/v] [/h]
    @echo Default: %my_name% [/app /b %bitness% /m %mode% /pts %pts% /bt %buildTools%]
    exit /B 0

:help
    call :usage
    echo.
    echo Targets:
    echo /app Build Hexter.exe application.
    echo /lib Build Hexter.dll library.
	echo.
	echo Options:
    echo /b Target bitness: 32^|64. Default: 64.
    echo /m Build mode: Debug^|Release. Default: Release.
    echo /rtl Statically include runtime libs. Increases file size but may be needed if a "VCRUNTIMExxx.dll not found Error" occurs on the target system.
    echo /pdb Include pdb symbols into release build. Default in debug mode. 
    echo /bt Custom path to Microsoft Visual Studio BuildTools
    echo /pts Platformtoolset. Defaults to "v142".
    echo.
    echo /v more verbose output
    echo /h print this

    endlocal
    exit /B 0
