#!/usr/bin/env sh
set -eu

build_dir="build"
cmake -S . -B "${build_dir}"
cmake --build "${build_dir}" -j
cp "${build_dir}/demo" demo
rm -rf "${build_dir}"
