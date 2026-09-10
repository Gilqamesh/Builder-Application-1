#!/usr/bin/env bash
set -euo pipefail

repo_root="${BUILDER_WORKSPACE_ROOT:?Set BUILDER_WORKSPACE_ROOT to the combined workspace}"
tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/builder-module-apply-language.XXXXXX")"
trap 'rm -r -- "$tmp_dir"' EXIT



last_line() {
    tail -n 1
}

run_eval_last_line() {
    local expression="$1"
    local output

    output="$("$repo_root/cli" m03gubnevc9vfwh0ka3iz3mjuf_builder_eval "$expression" 2>&1)"
    printf '%s\n' "$output" | last_line
}

expect_eval_last_line() {
    local name="$1"
    local expression="$2"
    local expected="$3"
    local actual

    actual="$(run_eval_last_line "$expression")"
    if [[ "$actual" != "$expected" ]]; then
        printf 'FAIL: %s\nExpected last line: %s\nActual last line: %s\n' "$name" "$expected" "$actual" >&2
        exit 1
    fi
}

expect_eval_last_line \
    "define/lambda/application composition" \
    '(define id (lambda (x) x)) (id "lambda-ok")' \
    "lambda-ok"

expect_eval_last_line \
    "module procedures are first-class values" \
    'kernel' \
    "kernel"

expect_eval_last_line \
    "first-class module procedure application returns capability records" \
    '(define f kernel) (record? (f))' \
    "true"

expect_eval_last_line \
    "lexical definitions shadow module names" \
    '(define kernel (lambda (x) x)) (kernel "shadow")' \
    "shadow"

expect_eval_last_line \
    "records and generic field lookup compose" \
    '(get (record "message" "record-ok") "message")' \
    "record-ok"

expect_eval_last_line \
    "if let begin and equality compose runtime values" \
    '(let ((x true)) (begin (equal? (list "a" x) (list "a" true)) (if x "branch-ok" "bad")))' \
    "branch-ok"

program_file="$tmp_dir/program.builder"
printf '(define loaded "file-ok")\nloaded\n' > "$program_file"
expect_eval_last_line \
    "evaluator programs load from files" \
    "(load \"$program_file\")" \
    "file-ok"

sha_file="$tmp_dir/payload.txt"
printf 'module apply payload' > "$sha_file"
sha_expected="$(sha256sum "$sha_file" | awk '{print $1}')"

expect_eval_last_line \
    "filesystem module returns callable capability record" \
    "(let ((fs (filesystem))) ((get fs \"exists\") \"$sha_file\"))" \
    "true"

expect_eval_last_line \
    "native capability values survive serialization" \
    "(let ((json (json)) (fs (filesystem))) (((get json \"deserialize_value\") ((get json \"serialize_value\") (get fs \"exists\"))) \"$sha_file\"))" \
    "true"

expect_eval_last_line \
    "builder_eval applies tool module through apply" \
    "(sha256sum \"$sha_file\" \"$sha_expected\")" \
    "unit"

graph_svg="$tmp_dir/download.svg"
expect_eval_last_line \
    "module-valued target arguments are accepted by module inspection tools" \
    "(module_graph download \"$graph_svg\")" \
    "$graph_svg"
if [[ ! -s "$graph_svg" ]]; then
    printf 'FAIL: module_graph did not write expected SVG at %s\n' "$graph_svg" >&2
    exit 1
fi

adapter_file="$tmp_dir/adapter.txt"
printf 'cli adapter payload' > "$adapter_file"
adapter_expected="$(sha256sum "$adapter_file" | awk '{print $1}')"
"$repo_root/cli" m03gagbhtbusaqidrtw6lnugr4_sha256sum "$adapter_file" "$adapter_expected" >/dev/null

repl_output="$(
    printf '(define id\n  (lambda (x)\n    x))\n(id "multi")\n:!!\n:history\n:1\n' \
        | "$repo_root/cli" m03gubnevc9uwrppfipf0i15zk_builder_repl 2>&1
)"
if [[ "$(printf '%s\n' "$repl_output" | grep -c '^> multi$')" -ne 2 ]]; then
    printf 'FAIL: repl history repeat did not re-run the previous command\n%s\n' "$repl_output" >&2
    exit 1
fi
if ! printf '%s\n' "$repl_output" | grep -Fq '1 (define id\n  (lambda (x)\n    x))'; then
    printf 'FAIL: repl history did not list multiline command\n%s\n' "$repl_output" >&2
    exit 1
fi
if ! printf '%s\n' "$repl_output" | grep -q '^> unit$'; then
    printf 'FAIL: repl history index did not invoke the multiline command\n%s\n' "$repl_output" >&2
    exit 1
fi

REPO_ROOT="$repo_root" python3 - <<'PY'
import os
import pty
import select
import subprocess
import time

repo_root = os.environ["REPO_ROOT"]
master_fd, slave_fd = pty.openpty()
process = subprocess.Popen(
    [os.path.join(repo_root, "cli"), "m03gubnevc9uwrppfipf0i15zk_builder_repl"],
    stdin=slave_fd,
    stdout=slave_fd,
    stderr=slave_fd,
    close_fds=True,
)
os.close(slave_fd)

buffer = bytearray()

def read_until(marker, timeout=20):
    deadline = time.time() + timeout
    while marker not in buffer:
        if time.time() > deadline:
            output = bytes(buffer).decode(errors="replace")
            raise RuntimeError(f"timed out waiting for {marker!r}\n{output}")
        readable, _, _ = select.select([master_fd], [], [], 0.1)
        if not readable:
            continue
        try:
            chunk = os.read(master_fd, 4096)
        except OSError:
            break
        if not chunk:
            break
        buffer.extend(chunk)

    return bytes(buffer)

try:
    read_until(b"> ")

    buffer.clear()
    os.write(master_fd, b"kern\t\n")
    read_until(b"kernel\r\n> ")

    buffer.clear()
    os.write(master_fd, b"builder_\t\t")
    read_until(b"builder_eval")
    read_until(b"builder_repl")

    buffer.clear()
    os.write(master_fd, (b"\x7f" * len(b"builder_")) + b"\x04")
    try:
        process.wait(timeout=10)
    except subprocess.TimeoutExpired as e:
        output = bytes(buffer).decode(errors="replace")
        raise RuntimeError(f"builder_repl did not exit after Ctrl-D\n{output}") from e
    if process.returncode != 0:
        output = bytes(buffer).decode(errors="replace")
        raise RuntimeError(f"builder_repl exited with {process.returncode}\n{output}")
finally:
    try:
        os.close(master_fd)
    except OSError:
        pass
    if process.poll() is None:
        process.terminate()
        process.wait(timeout=10)
PY

printf 'PASS: module apply language\n'
