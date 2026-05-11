#!/bin/bash
set -e
set -o pipefail

C906_YAML="solutions/pinevoice_fw_c906/package.yaml"
E907_YAML="solutions/pinevoice_fw_e907/package.yaml"

restore_yamls() {
    echo "Restore !!!"
    if [ -f "$C906_YAML.bak" ]; then
        rm -f "$C906_YAML"
        mv "$C906_YAML.bak" "$C906_YAML"
    fi
    if [ -f "$E907_YAML.bak" ]; then
        rm -f "$E907_YAML"
        mv "$E907_YAML.bak" "$E907_YAML"
    fi
}
trap restore_yamls EXIT
trap 'echo "package.sh: FAILED at line $LINENO (exit $?)" >&2' ERR

cp "$C906_YAML" "$C906_YAML.bak"
cp "$E907_YAML" "$E907_YAML.bak"

sed -i -E 's/^([[:space:]]*)(CONFIG_DEBUG[A-Z_]*[[:space:]]*:.*)/\1# \2/' "$C906_YAML"
sed -i -E 's/^([[:space:]]*)(CONFIG_DEBUG[A-Z_]*[[:space:]]*:.*)/\1# \2/' "$E907_YAML"

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
cp boards/bl606p_pinevoice_e907/bootimgs/bl606p_mfg_gu_*.bin .tmp/bl606p_mfg.bin
cp solutions/pinevoice_fw_e907/yoc_rfpa.bin .tmp/yoc_rfpa.bin
cp solutions/pinevoice_fw_e907/generated/littlefs.bin .tmp/littlefs.bin
pushd .tmp
COMMIT=$(git rev-parse --short=10 HEAD)
zip "../firmware_$COMMIT.zip" ./*
popd
rm -rf .tmp
