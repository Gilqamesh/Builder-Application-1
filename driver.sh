#!/usr/bin/env bash

set -euo pipefail

target_module=${1}
debug_flag="${2:-}"

latest_builder_dir=$(printf "%s\n" ./artifacts/builder/builder@* 2>/dev/null | sort | tail -n1 || true)
latest_builder_driver="$latest_builder_dir/builder_driver"

if [[ -z "$latest_builder_dir" || ! -x "$latest_builder_driver" ]]; then
  tmp_bin="$(mktemp -p /tmp builder_driver.XXXXXX)"

  exec_command=(clang++ -g -O3 -std=c++23 modules/builder/*.cpp -I. -o "$tmp_bin")
  echo "${exec_command[@]}"
  "${exec_command[@]}"

  exec_command=("$tmp_bin" . modules "$target_module" artifacts)
else
  exec_command=("$latest_builder_driver" . modules "$target_module" artifacts)
fi

[[ "$debug_flag" == "-g" ]] && exec_command=(gdb --args "${exec_command[@]}")

echo "${exec_command[@]}"
"${exec_command[@]}"

[[ -n "${tmp_bin:-}" ]] && rm -f "$tmp_bin"
