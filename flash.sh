#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")" && pwd)
port=
full=false
wait_for_port=false
boot2_isp=false

usage() {
  printf 'Usage: %s --port <serial-device> [--full] [--wait-for-port] [--boot2-isp]\n' "$0" >&2
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port)
      port=${2:-}
      shift 2
      ;;
    --full)
      full=true
      shift
      ;;
    --wait-for-port)
      wait_for_port=true
      shift
      ;;
    --boot2-isp)
      boot2_isp=true
      shift
      ;;
    *)
      usage
      exit 2
      ;;
  esac
done

if [[ -z "$port" ]]; then
  usage
  exit 2
fi

tool=${PINEVOICE_FLASH_TOOL:-}
if [[ -z "$tool" ]]; then
  case "$(uname -s)" in
    Darwin) tool="$root/tools/flashtool/bflb_iot_tool-macos" ;;
    Linux) tool="$root/tools/flashtool/bflb_iot_tool-ubuntu" ;;
  esac
fi
if [[ -z "$tool" || ! -x "$tool" ]]; then
  printf 'Run ./build.sh first, or set PINEVOICE_FLASH_TOOL to the Bouffalo flash tool.\n' >&2
  exit 1
fi
tool_dir=$(cd "$(dirname "$tool")" && pwd)

firmware="$root/solutions/pinevoice_fw_e907/yoc_rfpa.bin"
media="$root/solutions/pinevoice_fw_e907/generated/littlefs.bin"
partition="$root/boards/bl606p_pinevoice_e907/configs/partition.toml"
mfg="$root/boards/bl606p_pinevoice_e907/bootimgs/bl606p_mfg_gu_8476f7743.bin"
loader_config="$root/boards/bl606p_pinevoice_e907/configs/eflash_loader_cfg.ini"
tool_config_dir="$tool_dir/chips/bl606p/eflash_loader"
tool_config="$tool_config_dir/eflash_loader_cfg.ini"

[[ -f "$firmware" ]] || { printf 'Run ./build.sh first.\n' >&2; exit 1; }
[[ -f "$loader_config" ]] || { printf 'Missing PineVoice flash-tool configuration.\n' >&2; exit 1; }
[[ -d "$tool_config_dir" ]] || { printf 'Selected flash tool is missing its BL606P configuration directory.\n' >&2; exit 1; }

args=(--interface=uart --baudrate=2000000 --chipname=bl606p --firmware="$firmware" --pt="$partition" --port "$port")
if [[ "$full" == true ]]; then
  [[ -f "$media" ]] || { printf 'The build did not produce littlefs.bin.\n' >&2; exit 1; }
  args+=(--media="$media" --mfg="$mfg")
fi

if [[ "$wait_for_port" == true ]]; then
  printf 'Waiting for %s. Hold the center button and connect PineVoice.\n' "$port"
  until [[ -e "$port" ]]; do
    sleep 0.1
  done
fi

if [[ "$boot2_isp" == true ]]; then
  sed $'s/^boot2_isp_mode = 0\r$/boot2_isp_mode = 1\r/' "$loader_config" > "$tool_config"
else
  cp "$loader_config" "$tool_config"
fi

exec "$tool" "${args[@]}"
