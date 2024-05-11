ARG GLQUAKE
ARG WEBSOCKET_URL

FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    git ca-certificates build-essential cmake \
    python3 python3-pip libgl1-mesa-glx

WORKDIR /srv/

RUN git clone https://github.com/emscripten-core/emsdk.git

WORKDIR /srv/emsdk

RUN ./emsdk install latest
RUN ./emsdk activate latest

WORKDIR /srv/

RUN git clone https://github.com/ptitSeb/gl4es.git

WORKDIR /srv/emsdk

ARG GLQUAKE
ENV GLQUAKE=${GLQUAKE}

ARG WEBSOCKET_URL
ENV WEBSOCKET_URL=${WEBSOCKET_URL}

RUN . ./emsdk_env.sh && \
    cd ../gl4es && \
    mkdir -p build && \
    cd build && \
    emcmake cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DNOX11=ON -DNOEGL=ON -DSTATICLIB=ON && \
    make

COPY WinQuake /srv/WinQuake
COPY gnu.txt /srv/gnu.txt

WORKDIR /srv/emsdk

RUN . ./emsdk_env.sh && \
    cd ../WinQuake && \
    make -f Makefile.emscripten

FROM nginx:stable

COPY ./default.conf /etc/nginx/conf.d/default.conf

COPY --from=builder /srv/WinQuake/index.html /usr/share/nginx/html/index.html
COPY --from=builder /srv/WinQuake/index.js /usr/share/nginx/html/index.js
COPY --from=builder /srv/WinQuake/index.wasm /usr/share/nginx/html/index.wasm
COPY --from=builder /srv/WinQuake/index.data /usr/share/nginx/html/index.data
