@echo off

set target=hexter
set bitness=64
set mode=Release
set buildTools="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools"
set buildTools="D:\Programs\Microsoft Visual Studio\2019\BuildTools"

if not [%1]==[] (
    if [%1]==[/?] goto usage
    if [%1]==[/h] goto usage
    if [%1]==[/help] goto usage

    set target=%1
)
if not [%2]==[] set bitness=%2
if not [%3]==[] set mode=%3
if not [%4]==[] set buildTools=%4

set build_dir=build\%bitness%
if [%mode%]==[Debug] set build_dir=build\debug\%bitness%

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
    @echo Usage: %0 [%target%^|%target%_shared [32/64 [Debug^|Release [buildTools]]]
    @echo Default: %0 [%target% %bitness% %mode% %buildTools%]
    exit /B 1
