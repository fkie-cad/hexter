@echo off

set path=""
set label="Open with Hexter"

set prog_name=%~n0
set user_dir="%~dp0"
set verbose=0

GOTO :ParseParams

:ParseParams

    if [%1]==[/?] goto help
    if [%1]==[/h] goto help
    if [%1]==[/help] goto help

    IF "%~1"=="/p" (
		SET path=%~2
		SHIFT
		goto reParseParams
	)
    IF "%~1"=="/l" (
		SET label=%~2
		SHIFT
		goto reParseParams
	)
    IF "%~1"=="/v" (
		SET verbose=1
		goto reParseParams
	)
	
	:reParseParams
    SHIFT
	if [%1]==[] goto main

GOTO :ParseParams


:main

if ["%path%"] == [] goto usage
if ["%path%"] == [""] goto usage
if ["%label%"] == [] goto usage
if ["%label%"] == [""] goto usage

IF not exist "%path%" (
	echo Hexter not found at "%path%"!
	echo Place it there or adjust the path.
	exit /b 0
)

if [%verbose%]==[1] (
	echo path=%path%
	echo label=%label%
)

:add
	C:\Windows\System32\reg add "HKEY_CURRENT_USER\SOFTWARE\Classes\*\shell\%label%\Command" /t REG_SZ /d "cmd /k %path% -file \"%%1\""

exit /B 0


:usage
    @echo Usage: %prog_name% /p "c:\bin\hexter.exe" [/l "Open in Hexter"] [/v] [/h]
    exit /B 0

:help
	call :usage
    @echo /p Path to the hexter binary. Must not have spaces at the moment!
    @echo /l Label to show up in the context menu.
    @echo /v Verbose mode.
	@echo /h Print this.
    exit /B 0

:isDir
	setlocal
	set v=%~1
	if [%v:~0,-1%\] == [%v%] (
		exit /b 1
	) else (
		exit /b 0
	)
	endlocal