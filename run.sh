#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 2 ]; then
  echo "Usage: $0 <module> <binary_name> [args...]" >&2
  exit 1
fi

target_module="$1"
binary_name="$2"
shift 2
args=("$@")

artifacts_dir="artifacts"

latest_target_module="$(ls -d "${artifacts_dir}/${target_module}/${target_module}"@* 2>/dev/null | sort | tail -n1 || true)"

if [ -z "$latest_target_module" ]; then
  echo "No builds found for module '$target_module' in $artifacts_dir." >&2
  exit 1
fi

latest_binary="${latest_target_module}/link_module/${binary_name}"

if [ ! -x "$latest_binary" ]; then
  echo "No binary named '$binary_name' found for module '$target_module' in $artifacts_dir." >&2
  exit 1
fi

run_binary=("$latest_binary" "${args[@]}")
echo "${run_binary[@]}"
exec "${run_binary[@]}"
