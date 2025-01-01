#!/bin/bash

set -e

function teardown() {
    docker stop -t 0 quake-wasm-build >/dev/null 2>&1 || true
    popd >/dev/null 2>&1 || true
}
trap teardown exit
pushd "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1

REPO_NAME="$(git config --get remote.origin.url | cut -d ':' -f2- | cut -d '.' -f -1 | tr '[:upper:]' '[:lower:]')"

docker build --build-arg "WEBSOCKET_URL=${WEBSOCKET_URL:-}" --build-arg "GLQUAKE=${GLQUAKE:-}" -t "${REPO_NAME}:latest" -f ./Dockerfile .

docker run --rm -d --name quake-wasm-build "${REPO_NAME}:latest"

docker cp "quake-wasm-build:/usr/share/nginx/html/index.html" "./WinQuake/index.html"
docker cp "quake-wasm-build:/usr/share/nginx/html/index.js" "./WinQuake/index.js"
docker cp "quake-wasm-build:/usr/share/nginx/html/index.wasm" "./WinQuake/index.wasm"
docker cp "quake-wasm-build:/usr/share/nginx/html/index.data" "./WinQuake/index.data"

echo -e "\nAll done!"

echo -e "\nBuilt artifacts:\n"

ls -al WinQuake/index.data WinQuake/index.html WinQuake/index.js WinQuake/index.wasm
