#!/usr/bin/env bash
set -euo pipefail

repo_root="${BUILDER_WORKSPACE_ROOT:?Set BUILDER_WORKSPACE_ROOT to the combined workspace}"
tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/builder-eval-file-entry.XXXXXX")"
trap 'rm -r -- "$tmp_dir"' EXIT



last_line() {
    tail -n 1
}

run_eval_file() {
    local program_file="$1"

    env BUILDER_WORKSPACE_ROOT="$repo_root" "$repo_root/cli" m03gubnevc9vfwh0ka3iz3mjuf_builder_eval --file "$program_file" 2>&1
}

expect_eval_file_last_line() {
    local name="$1"
    local program_file="$2"
    local expected="$3"
    local actual

    actual="$(run_eval_file "$program_file" | last_line)"
    if [[ "$actual" != "$expected" ]]; then
        printf 'FAIL: %s\nExpected last line: %s\nActual last line: %s\n' "$name" "$expected" "$actual" >&2
        exit 1
    fi
}

record_program="$tmp_dir/record.builder"
cat > "$record_program" <<'BUILDER'
(let ((result (record "message" "file-record-ok")))
  (begin
    (record? result)
    (get result "message")))
BUILDER

result_channel="$tmp_dir/result.json"
record_output="$(
    env \
        BUILDER_WORKSPACE_ROOT="$repo_root" \
        BUILDER_RESULT_PATH="$result_channel" \
        "$repo_root/cli" m03gubnevc9vfwh0ka3iz3mjuf_builder_eval --file "$record_program" 2>&1
)"
record_last_line="$(printf '%s\n' "$record_output" | last_line)"
if [[ "$record_last_line" != "file-record-ok" ]]; then
    printf 'FAIL: file let/begin/record/get printed unexpected result\nExpected last line: file-record-ok\nActual output:\n%s\n' "$record_output" >&2
    exit 1
fi

python3 - "$result_channel" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as f:
    result = json.load(f)

if result != {"type_module": "string", "data": "file-record-ok"}:
    raise SystemExit(f"unexpected published result: {result!r}")
PY

payload="$tmp_dir/payload.txt"
printf 'file entry native capability payload' > "$payload"

capability_program="$tmp_dir/capability.builder"
cat > "$capability_program" <<BUILDER
(let ((fs (filesystem)))
  ((get fs "exists") "$payload"))
BUILDER

expect_eval_file_last_line \
    "file native capability call" \
    "$capability_program" \
    "true"

expression_output="$(
    env BUILDER_WORKSPACE_ROOT="$repo_root" "$repo_root/cli" m03gubnevc9vfwh0ka3iz3mjuf_builder_eval '(begin "expression-ok")' 2>&1
)"
expression_last_line="$(printf '%s\n' "$expression_output" | last_line)"
if [[ "$expression_last_line" != "expression-ok" ]]; then
    printf 'FAIL: expression compatibility path regressed\nExpected last line: expression-ok\nActual output:\n%s\n' "$expression_output" >&2
    exit 1
fi

relative_program="ws2/m03gubnevc9vfwh0ka3iz3mjuf_builder_eval/examples/programs.builder"
relative_output="$(
    cd "$repo_root"
    ./cli m03gubnevc9vfwh0ka3iz3mjuf_builder_eval --file "$relative_program" 2>&1
)"
relative_last_line="$(printf '%s\n' "$relative_output" | last_line)"
relative_expected='{"01_literal": hello from a file "02_list": [parse evaluate publish] "03_record_get": file "04_let_begin_record_get": file-record-ok "05_lambda_if_equal": ready "06_nested_record": {"details": {"name": builder_eval "state": ready} "steps": [parse evaluate publish] "summary": ready} "07_native_workspace_exists": true "08_serialized_capability_call": true}'
if [[ "$relative_last_line" != "$relative_expected" ]]; then
    printf 'FAIL: workspace-relative file path did not load from invocation root\nExpected last line: %s\nActual output:\n%s\n' \
        "$relative_expected" \
        "$relative_output" >&2
    exit 1
fi

printf 'PASS: builder_eval file entry\n'
