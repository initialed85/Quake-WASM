#!/bin/bash

set -e

function teardown() {
    popd >/dev/null 2>&1 || true
}
trap teardown exit
pushd "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1

if ! command -v cargo >/dev/null 2>&1; then
    echo "error: cargo command not found; (hint: https://doc.rust-lang.org/cargo/getting-started/installation.html)"
    exit 1
fi

if ! command -v devserver >/dev/null 2>&1; then
    echo "error: devserver command not found; (hint: cargo install devserver)"
    exit 1
fi

if ! test -e WinQuake/index.data &&
    test -e WinQuake/index.html &&
    test -e WinQuake/index.js &&
    test -e WinQuake/index.wasm; then
    echo "error: built artifacts not found; (hint: ./build.sh)"
    exit 1
fi

devserver \
    --header 'Cross-Origin-Opener-Policy: same-origin' \
    --header 'Cross-Origin-Embedder-Policy: require-corp' \
    --header 'Access-Control-Allow-Origin: *' \
    --header 'Access-Control-Allow-Methods: GET,OPTIONS' \
    --path ./WinQuake \
    --address 0.0.0.0:7070 \
    --noreload
