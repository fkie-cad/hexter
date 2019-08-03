@echo off

set target=hexter
set bitness=64
set mode=Release
set buildTools="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\"

if not [%1]==[] (
    if [%1]==[/?] goto usage
    if [%1]==[/h] goto usage
    if [%1]==[/help] goto usage

    set target=%1
)
if not [%2]==[] set bitness=%2
if not [%3]==[] set mode=%3
if not [%4]==[] set buildTools=%4

set build_dir=build%bitness%

echo target=%target%
echo bitness=%bitness%
echo mode=%mode%
echo build_dir=%build_dir%
echo buildTools=%buildTools%

:build
    cmd /k "mkdir %build_dir% & cd %build_dir% & %buildTools%"\VC\Auxiliary\Build\vcvars%bitness%.bat" & cmake .. -DCMAKE_BUILD_TYPE=%mode% -G "CodeBlocks - NMake Makefiles" & cmake --build . --config %mode% --target %target% & exit & cd .."
    exit /B 0

:usage
    @echo Usage: %0 [hexter/hexter_shared [32/64 [Debug/Release [buildTools]]]
    @echo Default: %0 [%target% %bitness% %mode% %buildTools%]
    exit /B 1
