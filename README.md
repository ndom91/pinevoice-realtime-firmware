# PineVoice SmartSpeaker SDK

This repository contains source code for Pine64's PineVoice. It is based on Bouffalo Lab's downstream fork of AliOS/YoC/YoCop.

# Development & Building

## Requirements

- Visual Studio Code (for development/editing)
  - Dev Container extension
- Docker (for building)

## Downloading

- Clone this repo: `git clone --recursive https://github.com/pine64/pinevoice_smartspeaker_sdk`
- Download [Bouffalo Dev Cube](http://files.pine64.org/tools/bouffalo/bflb_flashtool_bl606p_v190.tar.gz) and place it to `tools/flashtool` folder.

## Prepare environment

Start development environment with:
- VS Code Dev Container: Open `pinevoice-sdk.code-workspace`. Do not forget to configure `/dev/tty*` used by PineVoice in `.devcontainer/devcontainer.json`.
- Docker: Build and run `.devcontainer/Dockerfile`, while exposing `/dev/tty*` used by PineVoice, to access it from Docker.

## Building

- Release: run `package.sh`, result will be in `firmware_<commit>.zip`
- Debug:
  - Firstly, we need to compile C906 firmware. Go to `solutions/pinevoice_fw_c906` and run `./go`
  - Afterwards, we can compile main E907 firmware. `solutions/pinevoice_fw_e907` and run `./go` for long full compilation process, or `./build.sh` for shorter local build during development.

## Flashing

Turn off PineVoice, hold center ring button, and then turn on. Afterwards, run `./flash.sh`. Be fast, as there is timeout.

In `solutions/pinevoice_fw_e907` is `flash.sh` script. Usage: `./flash.sh <cli/-> <full/->`.

Add `cli` as first argument to launch command line after flash, add `full` as second argument to flash media data, mfg and so on. Additionally, if you want to update boot2, uncomment boot2 line (boot2 flashing works only through native UART, not USB VCP).

# License

AliOS/Xuantie-RTOS/Yocop is using Apache 2.0 license. Provided code credits goes to their respective owners: Alibaba/AliOS, Bouffalo Lab.

Code made by community for PineVoice is licensed with Apache 2.0 or MIT.
