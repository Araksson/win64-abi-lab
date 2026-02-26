@echo off
setlocal

REM ===============================
REM Case 03 - Boolean Representation
REM Target: x64 (Win64 ABI)
REM ===============================

set OPT=/O1

if "%1"=="O0" set OPT=/Od
if "%1"=="O1" set OPT=/O1
if "%1"=="O2" set OPT=/O2

if exist build rmdir /s /q build
mkdir build
cd build

clang-cl ^
    ..\main.cpp ^
    %OPT% ^
    /std:c++23 ^
    /W4 ^
    /Zi ^
    /fp:precise ^
    /Fa ^
    /Fe:case03.exe ^
    /link /INCREMENTAL:NO

echo.
echo Build complete (%OPT%)
echo.

endlocal
