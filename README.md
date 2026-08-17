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

The realtime endpoint can be set two ways. Prefer the runtime path: it survives
a reflash and does not need one.

### At runtime

The endpoint is stored in the `kv` partition. Open the serial console and set
it:

```
kv set    va_ws_host homeassistant.example.lan
kv setint va_ws_port 8080
```

The client re-reads both keys on each reconnect attempt, never mid-session. If
the device is disconnected and retrying, a change takes effect within about
five seconds. If it is already connected it keeps the old endpoint until the
connection drops, so follow the change with `reboot` to apply it immediately.
Either way no reflash is needed. Inspect or clear the keys with
`kv get va_ws_host` and `kv del va_ws_host`.

With a key unset, the firmware falls back to the value compiled into
`solutions/pinevoice_fw_e907/app/src/realtime/realtime_config.h`. An
out-of-range port falls back the same way, so a malformed entry cannot leave
the device pointed at an unusable port.

The console is the `PineVoice Console` USB CDC endpoint at 2000000 baud:

```bash
stty -F /dev/ttyACM0 2000000 raw -echo && cat /dev/ttyACM0
```

### Microphone capture

How the capture stream becomes the mono frame sent to the backend is tunable
from the same console, because the right answer depends on what the audio
driver actually delivers:

```
kv set    va_mic_mode ch0     # mono | ch0 | ch1 | avg | diff
kv setint va_mic_gain 4       # linear multiplier, 1..64
```

| Mode | Behaviour |
| --- | --- |
| `mono` | Reads one 320-byte frame and passes it through untouched. The default. |
| `ch0` / `ch1` | Reads a 3-channel frame, keeps that microphone only. |
| `avg` | Averages the two microphone channels. |
| `diff` | Subtracts them — for a pair wired anti-phase, where `avg` would cancel the voice instead of reinforcing it. |

The modes other than `mono` assume the driver interleaves
`mic1, mic2, reference` at `PCM_CHANNELS`. Measured on hardware that
assumption does not hold: the driver delivers a single channel, so those modes
consume three frames per frame emitted and audio arrives at roughly 40% of
realtime. Speech that arrives slower than it is spoken cannot be segmented by
a server-side VAD, which fails as an unresponsive assistant rather than as bad
audio. They are kept as diagnostics; `mono` is correct for this hardware.

`va_mic_gain` multiplies each sample with saturation and defaults to 1. It is a
tuning knob, not the level control: being a software multiply it scales the
noise floor along with the signal, so it buys headroom rather than
signal-to-noise. Capture level belongs in the codec, below.

### Capture gain

Two codec gains sit upstream, both reachable from the console:

```
voice gain  <id> <db>    # analog, 0 or 6..42 in steps of 3
voice dgain <id> <db>    # digital
```

Either without arguments prints all three channels.

The microphones are PDM (`board_audio_init` registers
`mic_type = INPUT_MIC_TYPE_DIGITAL`), so the **analog** gain does not reach
them: raising it from 27 dB to its 42 dB maximum changes the captured level not
at all. Only the **digital** stage applies, and the board never set it, leaving
capture about 33 dB below a reference Voice PE — quiet enough that a
server-side VAD finds nothing to segment, which presents as an assistant that
never answers rather than as audible problems.

`AUIDO_IN_GAIN_MIC_DIGITAL` in `board_audio.h` now applies 40 dB at boot.
Measured peaks by digital gain, holding everything else constant:

| Digital gain | Peak | Mean |
| --- | --- | --- |
| 0 dB (as shipped) | −46.8 dBFS | −66.0 dBFS |
| 20 dB | −29.9 dBFS | −49.5 dBFS |
| 40 dB | −15.3 dBFS | −34.6 dBFS |
| reference Voice PE | −13.0 dBFS | −32.0 dBFS |

`voice dgain` changes the register only and reverts on reboot; change the
constant to make it stick.

These are re-read about every two seconds, so a change applies without a
reboot. Both fall back to the compiled defaults when unset or out of range.

### At build time

`--host` and `--port` change the compiled-in fallback for a single build
without modifying the checked-in configuration:

```bash
./build.sh --host homeassistant.example.lan --port 8080
```

The default is `homeassistant.local:8080`.

### The endpoint must resolve over unicast DNS

PineVoice resolves the endpoint through lwIP's DNS client, which queries the
server it learned over DHCP. The firmware has **no mDNS resolver** —
`LWIP_MDNS_RESPONDER` only advertises the device, it does not look names up —
so a `.local` address never resolves, no matter how your LAN is configured.
Every attempt fails as a DNS timeout that the WebSocket layer reports as a
clean disconnect:

```
va_client.c[327]: Attempting WebSocket connection to homeassistant.local:8080
va_client.c[225]: Clean disconnect (no error)
```

Use a hostname your DNS server actually serves, or an IP address. This applies
to the built-in `homeassistant.local` default too — it only works on a network
where something provides unicast resolution for that name.

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
