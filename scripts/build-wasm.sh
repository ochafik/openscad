#!/bin/bash
set -euo pipefail

USE_CCACHE=${USE_CCACHE:-1}
EMSCRIPTEN_CCACHE_DIR=${EMSCRIPTEN_CCACHE_DIR:-$HOME/.ccache-em/}

BUILD_IMAGE=openscad/wasm-base:latest
if [[ "$USE_CCACHE" == "1" ]]; then
  mkdir -p "$EMSCRIPTEN_CCACHE_DIR"
  
  CCACHE_BUILD_IMAGE=openscad-wasm-ccache:local

  echo "
    FROM --platform=linux/amd64 $BUILD_IMAGE
    RUN apt update && \
        apt install -y ccache && \
        apt clean
  " | docker build --platform=linux/amd64 -t "$CCACHE_BUILD_IMAGE" -f - .

  BUILD_IMAGE=$CCACHE_BUILD_IMAGE
fi

docker run --rm -it --platform=linux/amd64 -v "$PWD:/src:rw" -v $EMSCRIPTEN_CCACHE_DIR:/root/.ccache:rw "$BUILD_IMAGE" \
  emcmake cmake -B submodules/openscad-wasm/build \
    -DEXPERIMENTAL=ON \
    "$@"

docker run --rm -it --platform=linux/amd64 -v "$PWD:/src:rw" "$BUILD_IMAGE" \
  cmake --build submodules/openscad-wasm/build -j
