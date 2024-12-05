@echo off
rem Mimics parts of the behavior of standard POSIX mkdir on Windows

setlocal enableextensions
set OPT=0

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
        if "%ARG_CHAR%" == "p" (
            set OPT=1
        ) else (
            echo Unknown option: %ARG_CHAR%
            exit /B 1
        )

        set ARG=%ARG:~1%
    goto :parse_arg_loop

:process
    if %OPT% == 1 if exist %1/ exit /B 0
    mkdir %1
    exit /B
