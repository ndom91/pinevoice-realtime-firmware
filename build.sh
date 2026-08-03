#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")" && pwd)
image=pinevoice-realtime-build

# Pine64's supplied BL606P toolchain is x86_64. Pinning the container keeps
# builds working on Apple Silicon hosts through Docker's amd64 emulation.
docker build --platform linux/amd64 --tag "$image" "$root/.devcontainer"
docker run --platform linux/amd64 --rm --volume "$root:/workspace" --workdir /workspace "$image" sh -c '
  if [ ! -x tools/flashtool/bflb_iot_tool-ubuntu ]; then
    curl -fsSL https://files.pine64.org/tools/bouffalo/bflb_flashtool_bl606p_v190.tar.gz \
      | tar xz -C tools/flashtool
  fi
  chmod +x tools/flashtool/bflb_iot_tool-macos tools/flashtool/bflb_iot_tool-ubuntu
  ./package.sh
'
