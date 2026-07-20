@echo off
setlocal enabledelayedexpansion
title ESP32 / Heltec OTA Upload Tool

REM ============================================================
REM  Portable OTA Upload Tool for ESP32 / Heltec boards
REM ============================================================
REM  Works on any Windows PC / any network. Settings (board IP,
REM  sketch folder, espota.py path) are saved next to this script
REM  in "ota_config.ini" so you only set them up once per machine.
REM
REM  Just double-click this file to start.
REM ============================================================

set CONFIG_FILE=%~dp0ota_config.ini
set BOARD_IP=
set SKETCH_DIR=
set ESPOTA=

call :load_config

:menu
cls
echo ===============================================
echo   ESP32 / Heltec OTA Upload Tool
echo ===============================================
echo   Powered by Mr. Pornchai Thong-in
echo   Computer Technology Faculty
echo   Mahasarakham Technical College
echo   Email: pornchai.tom@gmail.com
echo ===============================================
echo   Current settings:
echo     Board IP    : %BOARD_IP%
echo     Sketch dir  : %SKETCH_DIR%
echo     espota.py   : %ESPOTA%
echo ===============================================
echo.
echo   1. Upload firmware now
echo   2. Change board IP
echo   3. Change sketch folder (auto-detects espota.py + .bin)
echo   4. Open .ino file for editing (e.g. WiFi SSID/password)
echo   5. How to export the .bin file (instructions)
echo   6. Exit
echo.
set /p CHOICE=Select an option (1-6):

if "%CHOICE%"=="1" goto upload
if "%CHOICE%"=="2" goto change_ip
if "%CHOICE%"=="3" goto change_sketch
if "%CHOICE%"=="4" goto edit_ino
if "%CHOICE%"=="5" goto howto_export
if "%CHOICE%"=="6" goto :eof
goto menu


REM ============================================================
:load_config
REM ============================================================
if exist "%CONFIG_FILE%" (
    for /f "usebackq tokens=1,* delims==" %%A in ("%CONFIG_FILE%") do (
        if "%%A"=="BOARD_IP" set BOARD_IP=%%B
        if "%%A"=="SKETCH_DIR" set SKETCH_DIR=%%B
        if "%%A"=="ESPOTA" set ESPOTA=%%B
    )
)
goto :eof


REM ============================================================
:save_config
REM ============================================================
(
    echo BOARD_IP=%BOARD_IP%
    echo SKETCH_DIR=%SKETCH_DIR%
    echo ESPOTA=%ESPOTA%
) > "%CONFIG_FILE%"
goto :eof


REM ============================================================
:change_ip
REM ============================================================
echo.
echo Current IP: %BOARD_IP%
set /p NEWIP=Enter new board IP address (e.g. 192.168.1.50):
if not "%NEWIP%"=="" set BOARD_IP=%NEWIP%
call :save_config
echo.
echo Testing connection to %BOARD_IP% ...
ping -n 2 %BOARD_IP% >nul
if %ERRORLEVEL% EQU 0 (
    echo   OK - board responded to ping.
) else (
    echo   WARNING: ping failed. Check that the board is powered on
    echo   and connected to the same network as this PC.
)
echo.
pause
goto menu


REM ============================================================
:change_sketch
REM ============================================================
echo.
echo Enter the full path to your sketch folder
echo (the folder that contains the .ino file), for example:
echo   C:\Users\YourName\Documents\Arduino\MyProject
echo.
set /p NEWDIR=Sketch folder path:
if "%NEWDIR%"=="" goto menu

if not exist "%NEWDIR%" (
    echo.
    echo   ERROR: that folder does not exist. Please check the path.
    echo.
    pause
    goto menu
)

set SKETCH_DIR=%NEWDIR%
call :save_config

echo.
echo Searching for espota.py on this PC, please wait...
call :find_espota

echo.
pause
goto menu


REM ============================================================
:find_espota
REM Auto-detect espota.py under the Arduino15 packages folder
REM ============================================================
set FOUND_COUNT=0
if exist "%LOCALAPPDATA%\Arduino15\packages" (
    for /f "delims=" %%F in ('dir /s /b "%LOCALAPPDATA%\Arduino15\packages\espota.py" 2^>nul') do (
        set /a FOUND_COUNT+=1
        set "ESPOTA_!FOUND_COUNT!=%%F"
    )
)

if %FOUND_COUNT% EQU 0 (
    echo   Could not find espota.py automatically.
    echo   Make sure you have installed the ESP32 or Heltec board
    echo   package in Arduino IDE: Tools ^> Board ^> Boards Manager.
    set ESPOTA=
    call :save_config
    goto :eof
)

if %FOUND_COUNT% EQU 1 (
    set ESPOTA=!ESPOTA_1!
    echo   Found: !ESPOTA!
    call :save_config
    goto :eof
)

echo   Found multiple installations. Choose the one matching
echo   the board you selected in Arduino IDE ^(Tools ^> Board^):
for /l %%i in (1,1,%FOUND_COUNT%) do (
    echo   %%i. !ESPOTA_%%i!
)
set /p PICK=Enter number:
set ESPOTA=!ESPOTA_%PICK%!
call :save_config
goto :eof


REM ============================================================
:find_bin
REM Auto-detect the compiled .ino.bin under the sketch's build folder
REM Excludes bootloader.bin / partitions.bin / merged.bin / boot_app0.bin
REM ============================================================
set BIN_PATH=
set BIN_COUNT=0

if not exist "%SKETCH_DIR%\build" (
    goto :eof
)

for /f "delims=" %%F in ('dir /s /b "%SKETCH_DIR%\build\*.ino.bin" 2^>nul') do (
    set "NAME=%%~nxF"
    echo !NAME! | findstr /i "bootloader partitions merged" >nul
    if errorlevel 1 (
        set /a BIN_COUNT+=1
        set "BIN_!BIN_COUNT!=%%F"
    )
)

if %BIN_COUNT% EQU 1 (
    set BIN_PATH=!BIN_1!
    goto :eof
)

if %BIN_COUNT% GTR 1 (
    REM Multiple builds found - use the most recently modified one
    for /f "delims=" %%F in ('dir /s /b /o-d "%SKETCH_DIR%\build\*.ino.bin" 2^>nul') do (
        set "NAME=%%~nxF"
        echo !NAME! | findstr /i "bootloader partitions merged" >nul
        if errorlevel 1 (
            set BIN_PATH=%%F
            goto :eof
        )
    )
)
goto :eof


REM ============================================================
:check_python
REM Returns PYTHON_CMD = the working python command, or empty if none
REM ============================================================
set PYTHON_CMD=

python --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set PYTHON_CMD=python
    goto :eof
)

py --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set PYTHON_CMD=py
    goto :eof
)

python3 --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set PYTHON_CMD=python3
    goto :eof
)

goto :eof


REM ============================================================
:edit_ino
REM ============================================================
if "%SKETCH_DIR%"=="" (
    echo.
    echo   No sketch folder set yet. Choose option 3 first.
    echo.
    pause
    goto menu
)

set INO_FOUND=
for /f "delims=" %%F in ('dir /b "%SKETCH_DIR%\*.ino" 2^>nul') do set INO_FOUND=%SKETCH_DIR%\%%F

if "%INO_FOUND%"=="" (
    echo.
    echo   No .ino file found in: %SKETCH_DIR%
    echo.
    pause
    goto menu
)

echo.
echo Opening !INO_FOUND! in Notepad...
echo Remember: after editing WiFi SSID/password, you must
echo re-export the .bin file before uploading (option 5).
echo.
start notepad "!INO_FOUND!"
pause
goto menu


REM ============================================================
:howto_export
REM ============================================================
cls
echo ===============================================
echo   How to export the .bin file
echo ===============================================
echo.
echo   1. Open your sketch in Arduino IDE.
echo   2. Make sure the correct board is selected under
echo      Tools ^> Board (must match the board you upload to).
echo   3. Go to: Sketch ^> Export Compiled Binary
echo   4. Wait for it to finish compiling and exporting.
echo   5. A "build" folder will appear next to your .ino file,
echo      containing a file named "<sketchname>.ino.bin".
echo   6. Come back here and choose option 1 to upload.
echo.
echo   Note: a normal Upload (Ctrl+U) via USB does NOT create
echo   this file - you must use "Export Compiled Binary" instead.
echo.
pause
goto menu


REM ============================================================
:upload
REM ============================================================
cls
echo ===============================================
echo   Upload firmware via OTA
echo ===============================================
echo.

REM ---- Step 1: check Python ----
call :check_python
if "%PYTHON_CMD%"=="" (
    echo   ERROR: Python was not found on this PC.
    echo.
    echo   Please install Python from:
    echo     https://www.python.org/downloads/
    echo.
    echo   IMPORTANT: on the first install screen, check the box
    echo   that says "Add python.exe to PATH" before clicking Install.
    echo   After installing, close and reopen this tool.
    echo.
    pause
    goto menu
)
echo   [OK] Python found (%PYTHON_CMD%)

REM ---- Step 2: check board IP ----
if "%BOARD_IP%"=="" (
    echo   ERROR: no board IP set. Use option 2 first.
    pause
    goto menu
)
ping -n 1 %BOARD_IP% >nul
if %ERRORLEVEL% NEQ 0 (
    echo   WARNING: board did not respond to ping at %BOARD_IP%.
    echo   Check it is powered on and connected to this network.
    set /p CONTINUE=Continue anyway? [y/n]:
    if /i not "!CONTINUE!"=="y" goto menu
) else (
    echo   [OK] Board responds at %BOARD_IP%
)

REM ---- Step 3: check espota.py ----
if "%ESPOTA%"=="" (
    echo   espota.py path not set - searching automatically...
    call :find_espota
)
if "%ESPOTA%"=="" (
    echo   ERROR: espota.py could not be found.
    echo   Install the ESP32/Heltec board package in Arduino IDE first.
    pause
    goto menu
)
if not exist "%ESPOTA%" (
    echo   ERROR: saved espota.py path no longer exists:
    echo     %ESPOTA%
    echo   Re-detecting...
    call :find_espota
    if "%ESPOTA%"=="" (
        pause
        goto menu
    )
)
echo   [OK] espota.py found

REM ---- Step 4: check sketch folder ----
if "%SKETCH_DIR%"=="" (
    echo   ERROR: no sketch folder set. Use option 3 first.
    pause
    goto menu
)

REM ---- Step 5: find the .bin file ----
call :find_bin
if "%BIN_PATH%"=="" (
    echo   ERROR: no compiled .bin file found under:
    echo     %SKETCH_DIR%\build
    echo.
    echo   You need to export it first - see option 5 for steps
    echo   Arduino IDE ^> Sketch ^> Export Compiled Binary.
    echo.
    pause
    goto menu
)
echo   [OK] Found firmware: %BIN_PATH%

echo.
echo ===============================================
echo   Uploading to %BOARD_IP% ...
echo ===============================================
echo.

%PYTHON_CMD% "%ESPOTA%" -i %BOARD_IP% -p 3232 -f "%BIN_PATH%"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ===============================================
    echo   Upload successful!
    echo ===============================================
) else (
    echo.
    echo ===============================================
    echo   Upload failed. Common causes:
    echo   - Board is on a different network/WiFi than this PC
    echo   - Board's IP address changed ^(check Serial Monitor^)
    echo   - The .bin file is outdated ^(re-export after code changes^)
    echo ===============================================
)

echo.
pause
goto menu
