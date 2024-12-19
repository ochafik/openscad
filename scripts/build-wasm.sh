#!/bin/bash
#
# Builds OpenSCAD for WebAssembly using Docker & a prebuilt dev image.
#
# See:
# - https://github.com/openscad/openscad-wasm
#   For a barebones setup
# - https://github.com/openscad/openscad-playground
#   For a full-fledged example
#
# Example usage:
#
#   ./scripts/build-wasm.sh -DCMAKE_BUILD_TYPE=Debug -DEXPERIMENTAL=1
#   ( cd submodules/openscad-wasm && make test )
#
# If docker fails because of a platform mismatch, e.g. on Silicon Macs
# (build env currently only built for linux/amd64), register QEMU with Docker using:
#
#   docker run --privileged --rm tonistiigi/binfmt --install all
#
set -euo pipefail

BUILD_DIR=${BUILD_DIR:-submodules/openscad-wasm/build}
USE_CCACHE=${USE_CCACHE:-1}
EMSCRIPTEN_CCACHE_DIR=${EMSCRIPTEN_CCACHE_DIR:-$HOME/.ccache-em/}

BUILD_IMAGE=openscad/wasm-base:latest
if [[ "$USE_CCACHE" == "1" ]]; then
  mkdir -p "$EMSCRIPTEN_CCACHE_DIR"
  
  CCACHE_BUILD_IMAGE=openscad-wasm-ccache:local
  echo "
    FROM $BUILD_IMAGE
    RUN apt update && \
        apt install -y ccache && \
        apt clean
  " | docker build --platform=linux/amd64 -t "$CCACHE_BUILD_IMAGE" -f - .
  BUILD_IMAGE=$CCACHE_BUILD_IMAGE
fi

docker_run() {
  docker run --rm -it \
    --platform=linux/amd64 \
    -v "$PWD:/src:rw" \
    -v $EMSCRIPTEN_CCACHE_DIR:/root/.ccache:rw \
    "$BUILD_IMAGE" \
    "$@"
}

docker_run emcmake cmake -B "$BUILD_DIR" "$@"
docker_run cmake --build "$BUILD_DIR"
