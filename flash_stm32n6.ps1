# =========================
# STM32N6 AUTO DEPLOY SCRIPT
# Robust / CI-ready version
# =========================

$ErrorActionPreference = "Stop"

# -------- LOG SETUP --------
$LogFile = Join-Path $PSScriptRoot "deploy.log"
Start-Transcript -Path $LogFile -Append

try {
    Write-Host "===================================="
    Write-Host " STM32N6 DEPLOY START"
    Write-Host "===================================="

    # -------- PATHS --------
    $PROG_PATH = "C:\ST\STM32CubeProgrammer\bin"

    $SIGN_TOOL = Join-Path $PROG_PATH "STM32_SigningTool_CLI.exe"
    $PROG_CLI  = Join-Path $PROG_PATH "STM32_Programmer_CLI.exe"
    $LOADER    = Join-Path $PROG_PATH "ExternalLoader\MX25UM51245G_STM32N6570-NUCLEO.stldr"

    # -------- INPUT / TEMPLATE FILES --------
    $APPLI_IN = Join-Path $PSScriptRoot "Appli\Debug\PibHand_N6_Appli.bin"

    # Precompiled signed FSBL template (do not re-sign)
    $FSBL_TEMPLATE = Join-Path $PSScriptRoot "Template_XIP_FSBL_signed.bin"

    # -------- VALIDATION --------
    if (!(Test-Path $SIGN_TOOL)) { throw "Signing Tool nicht gefunden: $SIGN_TOOL" }
    if (!(Test-Path $PROG_CLI))  { throw "Programmer CLI nicht gefunden: $PROG_CLI" }
    if (!(Test-Path $FSBL_TEMPLATE)) { throw "Precompiled FSBL Template nicht gefunden: $FSBL_TEMPLATE" }
    if (!(Test-Path $APPLI_IN))   { throw "Appli nicht gefunden: $APPLI_IN" }

    # -------- STEP 1: SIGN APPLI ONLY --------
    Write-Host "`n=== 1. Signiere Appli (only) ==="

    $base = [IO.Path]::GetFileNameWithoutExtension($APPLI_IN)
    $APPLI_OUT = "${base}_signed.bin"

    & $SIGN_TOOL -bin $APPLI_IN -nk -t ssbl -o $APPLI_OUT -hv 2.3 -align
    if ($LASTEXITCODE -ne 0) { throw "Appli Signierung fehlgeschlagen" }

    # -------- STEP 2: FLASH (use template FSBL at 0x70000000) --------
    Write-Host "`n=== 2. Flashen (Dev Mode required) ==="

    & $PROG_CLI -c port=SWD mode=HOTPLUG -el $LOADER -hardRst -w $FSBL_TEMPLATE 0x70000000
    if ($LASTEXITCODE -ne 0) { throw "FSBL Flash fehlgeschlagen" }

    & $PROG_CLI -c port=SWD mode=HOTPLUG -el $LOADER -hardRst -w $APPLI_OUT 0x70100000
    if ($LASTEXITCODE -ne 0) { throw "Appli Flash fehlgeschlagen" }

    # -------- DONE --------
    Write-Host "`n===================================="
    Write-Host " DEPLOY SUCCESSFUL"
    Write-Host " Jumper jetzt auf LINKS stellen"
    Write-Host " Reset drücken"
    Write-Host "===================================="

}
catch {
    Write-Host "`n❌ ERROR: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Deploy abgebrochen!"
    exit 1
}
finally {
    Stop-Transcript
}