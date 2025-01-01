#!/bin/bash

set -e

function teardown() {
    popd >/dev/null 2>&1 || true
}
trap teardown exit
pushd "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1

if ! test -e emsdk; then
    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk

    ./emsdk install latest
    ./emsdk activate latest

    cd ..
fi

cd emsdk

# shellcheck disable=SC1091
source ./emsdk_env.sh

cd ..

if [[ "${GLQUAKE}" == "1" ]]; then
    if ! test -e gl4es; then
        git clone https://github.com/ptitSeb/gl4es.git
        cd gl4es

        mkdir -p build
        cd build

        emcmake cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DNOX11=ON -DNOEGL=ON -DSTATICLIB=ON
        make

        cd ../..
    fi
fi

cd WinQuake

# shellcheck disable=SC2068
DEBUG=${DEBUG:-} WEBSOCKET_URL=${WEBSOCKET_URL:-} GLQUAKE=${GLQUAKE:-} make -f Makefile.emscripten ${@}

cd ..

echo -e "\nAll done!"

if [[ ${*} == "clean" ]]; then
    exit 0
fi

echo -e "\nBuilt artifacts:\n"

ls -al WinQuake/index.data WinQuake/index.html WinQuake/index.js WinQuake/index.wasm
