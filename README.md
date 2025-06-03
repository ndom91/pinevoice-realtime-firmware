# PineVoice SmartSpeaker SDK

This repository contains source code for Pine64's PineVoice. It is based on Bouffalo Lab's downstream fork of AliOS/YoC/YoCop.

# Development & Building

## Requirements

- Visual Studio Code
  - Dev Container extension
- Docker

## Downloading

- Clone this repo: `git clone --recursive https://github.com/pine64/pinevoice_smartspeaker_sdk`
- Download [Bouffalo Dev Cube](http://files.pine64.org/tools/bouffalo/bflb_devcube_v180.tar.gz) and place it to `tools/flashtool` folder.

## Building

1. Open `pinevoice-sdk.code-workspace` within the Dev Container.
2. Open terminal in **MCU Project**
3. Execute `./go`

## Flashing

- With Desktop app (from host)
  - Open **BLDevCube** in `tools/flashtool` folder.
  - Select BL606P in chip selection dialog.
  - Set the parameters as follows:
    - TODO:
  - Select appropriate COM port.
  - Press Flash
  - If flashing fails, hold center button, reset the board and try again.
- With CLI tool (from container)
  - Assure that you have enabled `--device=/dev/ttyXXXX` in `.devcontainer/devcontainer.json` with proper path to connected device's tty.
  - Open **MCU Project** terminal within VS Code.
    - If you want to flash only firmware, execute only `./flash.sh`.
    - If you want to flash all partitions (audio, device tree etc.), use `./flash.sh all`.
  - If flashing fails, hold center button, reset the board and try again.

## Development

`./go` script compiles whole SDK, together with DSP project. For faster development, you can use `build.sh` script, which compiles only current project and skips checking changes in other components.

Additionally, `./flash.sh cli` will open PineVoice's UART console in `tio` after successful flashing of the firmware.