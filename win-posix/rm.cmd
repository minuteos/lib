@echo off
rem Mimics parts of the behavior of standard POSIX rm on Windows

setlocal enableextensions
set RECURSE=
set QUIET=
set FORCE=

:loop
    if "%1" == "" exit /B 0
    set ARG=%1

    if "%ARG:~0,1%" == "-" (
        call :parse_arg "%ARG:~1%"
    ) else (
        call :process "%ARG:/=\%"
    )
    if errorlevel 1 exit /B %ERRORLEVEL%

    shift
goto :loop

:parse_arg
    set ARG=%~1

    :parse_arg_loop
        if "%ARG%" == "" exit /B 0

        set ARG_CHAR=%ARG:~0,1%
        if "%ARG_CHAR%" == "r" (
            set RECURSE=/S
        ) else if "%ARG_CHAR%" == "f" (
            set QUIET=/Q
            set FORCE=/F
        ) else (
            echo Unknown option: %ARG_CHAR%
            exit /B 1
        )

        set ARG=%ARG:~1%
    goto :parse_arg_loop

:process
    for /D %%D in (%1) do (
        if exist %%D/ rmdir %RECURSE% %QUIET% %%D
    )

    for %%F in (%1) do (
        if exist %%F if not exist %%F/ del %FORCE% %%F
    )
    exit /B
