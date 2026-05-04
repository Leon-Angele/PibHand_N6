# =========================
# STM32N6 Deployment Script
# PowerShell Version
# =========================

# --- PFAD ANPASSEN ---
$PROG_PATH = "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin"

$SIGN_TOOL = Join-Path $PROG_PATH "STM32_SigningTool_CLI.exe"
$PROG_CLI  = Join-Path $PROG_PATH "STM32_Programmer_CLI.exe"
$LOADER    = "C:\ST\STM32CubeProgrammer\bin\ExternalLoader\MX25UM51245G_STM32N6570-NUCLEO.stldr"

# Input Binaries
$FSBL_IN  = "C:\Pfad\zu\deiner\FSBL.bin"
$APPLI_IN = "C:\Pfad\zu\deiner\Appli.bin"

# Output Signed Binaries
$FSBL_OUT  = "FSBL_signed.bin"
$APPLI_OUT = "Appli_signed.bin"

Write-Host "=== 1. Signiere Binaries ==="

& $SIGN_TOOL -bin $FSBL_IN -nk -of 0x70000000 -t fsbl -o $FSBL_OUT -hv 2.3 -align
& $SIGN_TOOL -bin $APPLI_IN -nk -of 0x70100000 -t ssbl -o $APPLI_OUT -hv 2.3 -align

Write-Host "`n=== 2. Flashen (Board muss im Dev-Mode sein) ==="

# FSBL flashen
& $PROG_CLI -c port=SWD mode=HOTPLUG -el $LOADER -hardRst -w $FSBL_OUT 0x70000000

# Application flashen
& $PROG_CLI -c port=SWD mode=HOTPLUG -el $LOADER -hardRst -w $APPLI_OUT 0x70100000

Write-Host "`n=== FERTIG! Jumper jetzt auf LINKS stellen und Reset drücken. ==="
Pause