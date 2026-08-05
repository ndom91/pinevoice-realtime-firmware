#!/bin/bash
set -e
set -o pipefail

C906_YAML="solutions/pinevoice_fw_c906/package.yaml"
E907_YAML="solutions/pinevoice_fw_e907/package.yaml"
REALTIME_CONFIG="solutions/pinevoice_fw_e907/app/src/realtime/realtime_config.h"
LOCK_FILE=".package.lock"

restore_files() {
    if [ -f "$C906_YAML.bak" ]; then
        rm -f "$C906_YAML"
        mv "$C906_YAML.bak" "$C906_YAML"
    fi
    if [ -f "$E907_YAML.bak" ]; then
        rm -f "$E907_YAML"
        mv "$E907_YAML.bak" "$E907_YAML"
    fi
    if [ -f "$REALTIME_CONFIG.bak" ]; then
        rm -f "$REALTIME_CONFIG"
        mv "$REALTIME_CONFIG.bak" "$REALTIME_CONFIG"
    fi
}

cleanup() {
    restore_files
    rm -f "$LOCK_FILE"
}

if ! command -v flock >/dev/null; then
    echo "flock is required to prevent concurrent builds" >&2
    exit 1
fi
exec 9>"$LOCK_FILE"
if ! flock -n 9; then
    echo "Another build is already running" >&2
    exit 1
fi

trap cleanup EXIT
trap 'echo "package.sh: FAILED at line $LINENO (exit $?)" >&2' ERR

cp "$C906_YAML" "$C906_YAML.bak"
cp "$E907_YAML" "$E907_YAML.bak"
cp "$REALTIME_CONFIG" "$REALTIME_CONFIG.bak"

case "${PINEVOICE_BUILD_MODE:-release}" in
    release)
        sed -i -E 's/^([[:space:]]*)(CONFIG_DEBUG[A-Z_]*[[:space:]]*:.*)/\1# \2/' "$C906_YAML"
        sed -i -E 's/^([[:space:]]*)(CONFIG_DEBUG[A-Z_]*[[:space:]]*:.*)/\1# \2/' "$E907_YAML"
        ;;
    debug)
        ;;
    *)
        echo "PINEVOICE_BUILD_MODE must be release or debug" >&2
        exit 2
        ;;
esac

if [ -n "${PINEVOICE_HOME_ASSISTANT_HOST:-}" ]; then
    if [[ ! "$PINEVOICE_HOME_ASSISTANT_HOST" =~ ^[A-Za-z0-9][A-Za-z0-9.-]*$ ]]; then
        echo "PINEVOICE_HOME_ASSISTANT_HOST is not a valid hostname" >&2
        exit 2
    fi
    sed -i -E "s|^#define VA_WS_HOST \".*\"$|#define VA_WS_HOST \"${PINEVOICE_HOME_ASSISTANT_HOST}\"|" "$REALTIME_CONFIG"
fi
if [ -n "${PINEVOICE_HOME_ASSISTANT_PORT:-}" ]; then
    if [[ ! "$PINEVOICE_HOME_ASSISTANT_PORT" =~ ^[1-9][0-9]{0,4}$ ]] || ((10#$PINEVOICE_HOME_ASSISTANT_PORT > 65535)); then
        echo "PINEVOICE_HOME_ASSISTANT_PORT must be between 1 and 65535" >&2
        exit 2
    fi
    sed -i -E "s|^#define VA_WS_PORT [0-9]+$|#define VA_WS_PORT ${PINEVOICE_HOME_ASSISTANT_PORT}|" "$REALTIME_CONFIG"
fi

pushd solutions/pinevoice_fw_c906
./go clean
./go
popd
pushd solutions/pinevoice_fw_e907
./go clean
./go
popd
rm -rf .tmp
mkdir .tmp
cp boards/bl606p_pinevoice_e907/configs/partition.toml .tmp/partition.toml
cp boards/bl606p_pinevoice_e907/configs/chip_params.dts .tmp/chip_params.dts
cp boards/bl606p_pinevoice_e907/bootimgs/boot2_isp_release.bin .tmp/boot2.bin
cp boards/bl606p_pinevoice_e907/bootimgs/bl606p_mfg_gu_8476f7743.bin .tmp/bl606p_mfg.bin
cp solutions/pinevoice_fw_e907/yoc_rfpa.bin .tmp/yoc_rfpa.bin
cp solutions/pinevoice_fw_e907/generated/littlefs.bin .tmp/littlefs.bin
pushd .tmp
COMMIT=$(git rev-parse --short=10 HEAD)
zip "../firmware_$COMMIT.zip" ./*
popd
rm -rf .tmp
