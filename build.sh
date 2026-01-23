#!/usr/bin/env bash
set -euo pipefail

build_dir="build"
cmake -S . -B "${build_dir}"
cmake --build "${build_dir}" -j
rm -rf "${build_dir}"
