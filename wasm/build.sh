#!/usr/bin/env bash
set -euo pipefail

mkdir -p web/src/generated
mkdir -p web/public

emcc \
  -std=c++23 \
  -O3 \
  -I. \
  -Ithird_party/eigen3 \
  delaunay/delaunay.cpp \
  delaunay/builder.cpp \
  wasm/api.cpp \
  -sMODULARIZE=1 \
  -sEXPORT_ES6=1 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXPORTED_FUNCTIONS='["_malloc","_free","_dlny_build","_dlny_vertex_count","_dlny_tet_count","_dlny_get_vertices","_dlny_get_indices"]' \
  -sEXPORTED_RUNTIME_METHODS='["HEAPF64","HEAPU32"]' \
  -o web/src/generated/delaunay3d.js

if [ -f web/src/generated/delaunay3d.wasm ]; then
  mv web/src/generated/delaunay3d.wasm web/public/delaunay3d.wasm
fi
