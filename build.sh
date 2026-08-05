#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")" && pwd)
image=pinevoice-realtime-build
mode=release
host=
port=

usage() {
  printf 'Usage: %s [--release|--debug] [--host <hostname>] [--port <port>]\n' "$0" >&2
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --release)
      mode=release
      shift
      ;;
    --debug)
      mode=debug
      shift
      ;;
    --host)
      host=${2:-}
      if [[ -z "$host" || ! "$host" =~ ^[A-Za-z0-9][A-Za-z0-9.-]*$ ]]; then
        usage
        exit 2
      fi
      shift 2
      ;;
    --port)
      port=${2:-}
      if [[ ! "$port" =~ ^[1-9][0-9]{0,4}$ ]] || ((10#$port > 65535)); then
        usage
        exit 2
      fi
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      usage
      exit 2
      ;;
  esac
done

# Pine64's supplied BL606P toolchain is x86_64. Pinning the container keeps
# builds working on Apple Silicon hosts through Docker's amd64 emulation.
docker build --platform linux/amd64 --tag "$image" "$root/.devcontainer"
docker_args=(--platform linux/amd64 --rm --volume "$root:/workspace" --workdir /workspace)
docker_args+=(--env "PINEVOICE_BUILD_MODE=$mode")
if [[ -n "$host" ]]; then
  docker_args+=(--env "PINEVOICE_HOME_ASSISTANT_HOST=$host")
fi
if [[ -n "$port" ]]; then
  docker_args+=(--env "PINEVOICE_HOME_ASSISTANT_PORT=$port")
fi

docker run "${docker_args[@]}" "$image" sh -c '
  if [ ! -x tools/flashtool/bflb_iot_tool-ubuntu ]; then
    curl -fsSL https://files.pine64.org/tools/bouffalo/bflb_flashtool_bl606p_v190.tar.gz \
      | tar xz -C tools/flashtool
  fi
  chmod +x tools/flashtool/bflb_iot_tool-macos tools/flashtool/bflb_iot_tool-ubuntu
  ./package.sh
'
