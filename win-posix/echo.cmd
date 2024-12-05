@echo off
rem Very limited POSIX-style echo imitation, supports printing multiline env variables

setlocal enableextensions enabledelayedexpansion

set ARG=%~1
if "%ARG:~0,1%" == "$" (
    echo !%ARG:~1%!
) else (
    echo "%*"
)
