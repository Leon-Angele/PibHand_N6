# STM32N6 Deployment (simplified)

This repository uses a precompiled, signed FSBL template: `Template_XIP_FSBL_signed.bin`.

Workflow:

- Build the application: `Appli\Debug\PibHand_N6_Appli.bin`
- Sign only the application using the STM32 Signing Tool (creates `PibHand_N6_Appli_signed.bin`):

- Example (from project root):

		cmd.exe /C "STM32_SigningTool_CLI.exe -bin "Appli\\Debug\\PibHand_N6_Appli.bin" -nk -t ssbl -o "Appli\\Debug\\PibHand_N6_Appli_signed.bin" -hv 2.3 -align"

- Flashing:

	- Flash `Template_XIP_FSBL_signed.bin` to `0x70000000` (precompiled FSBL)
	- Flash the signed application `PibHand_N6_Appli_signed.bin` to `0x70100000`

You can use `flash_stm32n6.ps1` from the project root; it will sign the Appli and then flash the template FSBL to `0x70000000` and the signed Appli to `0x70100000`.

Requirements:

- `STM32_SigningTool_CLI.exe` and `STM32_Programmer_CLI.exe` in your PATH or installed in the usual STM32CubeProgrammer location.
- Board in Dev/Hotplug mode for flashing.

Notes:

- Do NOT re-sign the FSBL; use the provided `Template_XIP_FSBL_signed.bin` as-is.
- Adjust paths in `flash_stm32n6.ps1` if your build output directory differs.