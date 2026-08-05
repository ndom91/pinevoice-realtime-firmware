# PineVoice Realtime firmware

Native firmware for the Pine64 PineVoice that connects to the Voice PE Realtime
Home Assistant add-on. It is based on Pine64's BL606P SDK, not ESPHome.

The E907 application retains the PineVoice board support, Wi-Fi provisioning,
buttons, physical mute, LEDs, audio driver, and flash layout. Its Wyoming
satellite has been replaced with a persistent WebSocket client compatible with
the Voice PE Realtime protocol:

- 16 kHz mono PCM microphone audio is sent as binary WebSocket frames after
  `wake`.
- Reply audio is received as 24 kHz mono PCM and played through PineVoice's
  raw-audio FIFO.
- `start`, `wake`, and `interrupt` JSON control messages and the backend phase
  messages use the same protocol as `voicepe-realtime-firmware`.
- The C906 wake-word core loads the copied `Hey Leonard` model from the
  filesystem image.

## Acknowledgments

This firmware is built on the
[PineVoice Smart Speaker SDK](https://github.com/pine64/pinevoice_smartspeaker_sdk)
from Pine64 and the original
[Voice PE Realtime firmware](https://github.com/TristanBrotherton/voicepe-realtime-firmware)
by Tristan Brotherton.

## Configure

By default, the firmware connects to `homeassistant.local:8080`. Override the
hostname or port for one build with `--host` and `--port`; this does not modify
the checked-in configuration.

## Build

Docker is required. Build both BL606P images and assemble a release archive:

```bash
./build.sh
```

`./build.sh` creates a release build by default. Use `--debug` to retain debug
logging, or specify a Home Assistant endpoint that your PineVoice can resolve:

```bash
./build.sh --debug --host home-assistant.example.com --port 8080
```

The resulting files are:

- `solutions/pinevoice_fw_e907/yoc_rfpa.bin`: E907 application image
- `solutions/pinevoice_fw_e907/generated/littlefs.bin`: filesystem image,
  including the wake-word model
- `firmware_<commit>.zip`: release bundle

## Flash

`./build.sh` downloads Pine64's Bouffalo flash tool into `tools/flashtool`.
Turn off PineVoice, hold its centre ring button, turn it on, then run:

```bash
./flash.sh --port /dev/ttyUSB0 --full
```

Use the device path exposed by your host, such as `/dev/cu.usbmodem...` on
macOS. Prefer the `cu.*` callout device over its `tty.*` counterpart for
flashing. `--full` writes the application, filesystem, and manufacturing image;
omit it for an application-only update.

If the bootloader USB device disappears before the flash tool starts, launch
this command before powering PineVoice on, then hold the centre button and
connect it:

```bash
./flash.sh --port /dev/cu.usbmodem20221234561 --full --wait-for-port
```

If a normally booted PineVoice exposes its `PineVoice Console` USB endpoint,
Boot2 ISP can request the resident firmware to enter the flash loader:

```bash
./flash.sh --port /dev/cu.usbmodem20221234561 --full --wait-for-port --boot2-isp
```

`--boot2-isp` does not repair a `CherryUSB_CDC_DEMO` centre-button recovery
endpoint that does not acknowledge the loader handshake.

Set `PINEVOICE_FLASH_TOOL` only when using a custom Bouffalo flash-tool binary.

## Status

This is a native port of the realtime audio transport. It does not expose
ESPHome entities or support ESPHome OTA, because PineVoice is a BL606P device.
USB serial flashing is the supported installation path until native OTA is
implemented and tested on the hardware.
