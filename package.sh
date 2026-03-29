#!/bin/bash

pushd solutions/pinevoice_fw_c906
[ "$1" = "clean" ] && ./go clean
./go
popd
pushd solutions/pinevoice_fw_e907
[ "$1" = "clean" ] && ./go clean
./go
popd
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