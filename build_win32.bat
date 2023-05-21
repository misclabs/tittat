@echo off

set Platform=win32

if "%1"=="release" (
	set BuildType=release
) else if "%1"=="debug" (
	set BuildType=debug
) else (
	echo "BuildType is required"
	exit /B 1
)

if "%2"=="clean" set Clean=1
if "%2"=="configure" set Configure=1
if "%2"=="build" set Build=1
if "%2"=="run" set Run=1

set ProjectDir=%~dp0
set BuildDir=%ProjectDir%build\%Platform%\%BuildType%\
if "%BuildType%"=="debug" set BinDir=%BuildDir%runtime\Debug\
if "%BuildType%"=="release" set BinDir=%BuildDir%runtime\Release\

if defined Clean (
	rmdir /S /Q %BuildDir%
)

if defined Configure (
	cmake -S %ProjectDir% -B %BuildDir% -DCMAKE_BUILD_TYPE=%BuildType%
)

if defined Build (
	cmake --build %BuildDir% --config=%BuildType%
)

if defined Run (
	pushd %BuildDir%
	%BinDir%tittat
	popd
)