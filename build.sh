#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
  echo "Usage: $0 <module_name>" >&2
  exit 1
fi

target_module="$1"
modules_dir="modules"
artifacts_dir="artifacts"
builder_module_dir="$modules_dir/builder"
builder_driver="$artifacts_dir/builder_driver"

mkdir -p "$artifacts_dir"

if [ ! -x "$builder_driver" ]; then
  shopt -s nullglob
  sources=("$builder_module_dir"/*.cpp)
  (( ${#sources[@]} )) || {
    echo "No builder sources found" >&2
    exit 1
  }
  build_builder_driver=(clang++ -std=c++23 "${sources[@]}" -I"$(dirname "$modules_dir")" -o "$builder_driver")
  echo "${build_builder_driver[@]}"
  "${build_builder_driver[@]}"
fi

run_builder_driver=("$builder_driver" "$modules_dir" "$target_module" "$artifacts_dir")
echo "${run_builder_driver[@]}"
exec "${run_builder_driver[@]}"
