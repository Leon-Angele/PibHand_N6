# Deployment Guide: STM32N6 Standalone Boot (NUCLEO-N657X0-Q)

Dieser Guide beschreibt, wie man die Binaries signiert und in den externen Octo-Flash flasht, damit das Board ohne Debugger startet.

## 1. Hardware-Vorbereitung
*   **Flash-Vorgang:** Jumper **BOOT1** und **BOOT2** nach **RECHTS** (Development Mode).
*   **Nach dem Flashen:** Jumper **BOOT1** und **BOOT2** nach **LINKS** (Flash Boot Mode).

---

## 2. Signierung & Flash-Skript (Windows CMD/Batch)

Kopiere diesen Block in eine `.bat` Datei oder führe die Befehle in der Eingabeaufforderung aus. 

> **Hinweis:** Passe die Pfade zu deinen `.bin` Dateien und dem STM32CubeProgrammer an.
```batch
@echo off
:: --- PFADE ANPASSEN ---
set "PROG_PATH=C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin"
set "LOADER=%PROG_PATH%\ExternalLoader\MX25UM51245G_STM32N6570-NUCLEO.stldr"

set "FSBL_IN=C:\Pfad\zu\deiner\FSBL.bin"
set "APPLI_IN=C:\Pfad\zu\deiner\Appli.bin"
set "NETWORK_HEX=C:\Pfad\zu\deiner\network_data.hex"

:: Pfad zum Signing Tool hinzufügen
set PATH=%PATH%;%PROG_PATH%

echo === 1. Signiere Binaries ===
STM32_SigningTool_CLI.exe -bin "%FSBL_IN%" -nk -of 0x70000000 -t fsbl -o FSBL_signed.bin -hv 2.3 -align
STM32_SigningTool_CLI.exe -bin "%APPLI_IN%" -nk -of 0x70100000 -t ssbl -o Appli_signed.bin -hv 2.3 -align

echo.
echo === 2. Flashen (Board muss im Dev-Mode sein) ===
:: FSBL flashen
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -el "%LOADER%" -hardRst -w FSBL_signed.bin 0x70000000

:: Application flashen
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -el "%LOADER%" -hardRst -w Appli_signed.bin 0x70100000

:: Network Data flashen (Adresse im Hex enthalten)
STM32_Programmer_CLI.exe -c port=SWD mode=HOTPLUG -el "%LOADER%" -hardRst -w "%NETWORK_HEX%"

echo === FERTIG! Jumper jetzt auf LINKS stellen und Reset drücken. ===
pause