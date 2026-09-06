#!/usr/bin/env python3
"""Build and measure the complete headless renderer dependency closure on Linux."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import math
from pathlib import Path
import platform
import re
import shutil
import statistics
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--size", type=int, default=128)
    parser.add_argument("--warmup", type=int, default=3)
    parser.add_argument("--samples", type=int, default=20)
    parser.add_argument("--runs", type=int, default=5)
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    if platform.system() != "Linux":
        parser.error("this benchmark's peak RSS measurement currently requires Linux")
    if min(args.size, args.samples, args.runs, args.jobs) < 1 or args.warmup < 0:
        parser.error("size, samples, runs and jobs must be positive; warmup must be nonnegative")
    root = args.workspace_root.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    compiler = shutil.which(args.compiler)
    if compiler is None:
        parser.error("compiler was not found")
    compiler = str(Path(compiler).absolute())
    version = subprocess.check_output([compiler, "--version"], text=True).splitlines()[0]
    owner = Path(__file__).resolve().parents[1]
    modules = {path.name: path.resolve() for ws in root.glob("ws[0-9]*") if ws.is_dir() for path in ws.iterdir() if path.is_dir()}
    if modules.get(owner.name) != owner:
        parser.error("run.py must belong to the selected workspace's renderer module")

    # This dedicated path handles the headless modules' ordinary C++ sources and
    # literal includes. It does not reuse Builder libraries or emulate producers.
    sources = set()
    files = set()
    pending = []
    visited_modules = set()
    includes = re.compile(r'^\s*#\s*include\s*([<"])([^>"\n]+)[>"]', re.MULTILINE)

    def add_module(name):
        if name in visited_modules:
            return
        visited_modules.add(name)
        for source in modules[name].glob("*.cpp"):
            if source.name not in ("builder.cpp", "cli.cpp"):
                sources.add(source)
                pending.append((name, source))

    benchmark = owner / "cli/benchmark.cpp"
    validation = owner / "test/public_api.cpp"
    profiling_name = "m03gtjqkhqacstl3luv2ojsz3q_profiling"
    profiling_validation = modules[profiling_name] / "test/public_api.cpp"
    pending.extend([(owner.name, benchmark), (owner.name, validation), (profiling_name, profiling_validation)])
    add_module(owner.name)
    add_module(profiling_name)
    while pending:
        name, path = pending.pop()
        path = path.resolve()
        key = (name, path)
        if key in files:
            continue
        if not path.is_file() or not path.is_relative_to(modules[name]):
            raise RuntimeError(f"unsupported or missing headless source include: {path}")
        files.add(key)
        for delimiter, include in includes.findall(path.read_text()):
            dependency, separator, relative = include.partition("/")
            if dependency in modules and separator:
                add_module(dependency)
                pending.append((dependency, modules[dependency] / relative))
            elif delimiter == '"':
                pending.append((name, path.parent / include))
            elif re.match(r"m[0-9a-z]{25}_", dependency):
                raise RuntimeError(f"module include not found: {include}")

    flags = ["-std=c++23", "-O2", "-g", "-DNDEBUG", "-Wall", "-Wextra"]
    digest = hashlib.sha256(json.dumps([compiler, version, flags]).encode())
    ordered_files = sorted(files, key=lambda entry: (entry[0], str(entry[1])))
    for name, path in ordered_files:
        digest.update(str(Path(name) / path.relative_to(modules[name])).encode())
        digest.update(path.read_bytes())
    source_digest = digest.hexdigest()
    build = output / "build" / source_digest[:20]
    snapshot = build / "source"
    for name, path in ordered_files:
        target = snapshot / name / path.relative_to(modules[name])
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(path.read_bytes())
    extra_sources = [(owner.name, benchmark, "benchmark"), (owner.name, validation, "renderer-tests"), (profiling_name, profiling_validation, "profiling-tests")]
    commands = []
    objects = []
    targets = {}
    for source in sorted(sources):
        name = next(name for name in visited_modules if source.is_relative_to(modules[name]))
        relative = Path(name) / source.relative_to(modules[name])
        obj = build / "objects" / relative.with_suffix(".o")
        obj.parent.mkdir(parents=True, exist_ok=True)
        commands.append({"directory": str(build), "file": str(snapshot / relative), "arguments": [compiler, *flags, "-I", str(snapshot), "-c", str(snapshot / relative), "-o", str(obj)]})
        objects.append(str(obj))
    for name, source, target in extra_sources:
        relative = Path(name) / source.relative_to(modules[name])
        obj = build / (target + ".o")
        targets[target] = obj
        commands.append({"directory": str(build), "file": str(snapshot / relative), "arguments": [compiler, *flags, "-I", str(snapshot), "-c", str(snapshot / relative), "-o", str(obj)]})
    (output / "compile_commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    print(f"Compiling {len(commands)} translation units with -O2, including all measured dependency implementations", flush=True)
    def compile_source(command):
        result = subprocess.run(command["arguments"], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError(result.stdout)
        return result.stdout
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        logs = list(pool.map(compile_source, commands))
    (output / "build.log").write_text("".join(logs))
    for name, obj in targets.items():
        subprocess.run([compiler, str(obj), *objects, "-o", str(build / name)], check=True)
    subprocess.run([str(build / "profiling-tests")], check=True)
    subprocess.run([str(build / "renderer-tests")], check=True)

    # A separate optimized translation unit proves default draw instantiation does
    # not pull clock reads or recording into the normal renderer.
    probe = build / "disabled.cpp"
    probe.write_text(f'''#include <{owner.name}/software_renderer.h>
using namespace {owner.name};
extern "C" void disabled_draw(software_renderer_t<>& renderer, const camera_t& camera, const render_item_t& item) {{ renderer.draw(camera, item); }}
''')
    subprocess.run([compiler, *flags, "-I", str(snapshot), "-S", str(probe), "-o", str(build / "disabled.s")], check=True)
    assembly = (build / "disabled.s").read_text()
    # Examine emitted instructions, excluding debug strings containing header names.
    instructions = "\n".join(line for line in assembly.splitlines() if re.search(r"\b(call|jmp)\b", line))
    demangled = subprocess.check_output(["c++filt"], input=instructions, text=True)
    if re.search(r"steady_clock::now|clock_gettime|uncaught_exceptions", demangled) or any("profiling::profiler_t<" in line and re.search(r">::(?:begin|end|scope)(?:<|\()", line) for line in demangled.splitlines()):
        raise RuntimeError("disabled renderer unexpectedly calls profiling machinery")

    missing_formatter = f"""#include <{profiling_name}/api.h>
#include <array>
struct metrics_t {{ int m_count = 0; }};
int main() {{
    std::array<{profiling_name}::record_t<metrics_t>, 1> storage;
    {profiling_name}::profiler_t<metrics_t> profiler(storage);
}}
"""
    rejected = subprocess.run([compiler, *flags, "-I", str(snapshot), "-fsyntax-only", "-x", "c++", "-"], input=missing_formatter, text=True, capture_output=True)
    if rejected.returncode == 0 or "enabled profiling requires a payload formatter" not in rejected.stderr:
        raise RuntimeError("missing enabled payload formatter was not diagnosed")
    (output / "missing-formatter.log").write_text(rejected.stderr)

    cpu = next((line.split(":", 1)[1].strip() for line in Path("/proc/cpuinfo").read_text().splitlines() if line.startswith("model name")), platform.machine())
    revisions = {}
    for name in sorted(visited_modules):
        result = subprocess.run(["git", "-C", str(modules[name]), "rev-parse", "HEAD"], text=True, capture_output=True)
        status = subprocess.run(["git", "-C", str(modules[name]), "status", "--porcelain", "--", "."], text=True, capture_output=True)
        revisions[name] = {"revision": result.stdout.strip() if result.returncode == 0 else None, "modified": bool(status.stdout.strip()) if status.returncode == 0 else None}
    metadata = {"workload_version": 1, "source_digest": source_digest, "compiler": version, "compiler_path": compiler, "flags": flags, "cpu": cpu, "platform": platform.platform(), "size": args.size, "warmup_per_run": args.warmup, "samples_per_run": args.samples, "runs": args.runs, "scope": "frame scope, full color/depth clears, fixed draw sequence; reset, setup, comparison, reporting excluded", "peak_rss_scope": "Linux VmHWM for benchmark address space, both policies, setup, warmup and reporting included", "modules": revisions}
    results = {"metadata": metadata, "workloads": {}}
    for workload in ["textured_fill", "depth_overdraw", "many_draws", "clipping"]:
        capture = subprocess.check_output([str(build / "benchmark"), workload, str(args.size), str(args.warmup), str(args.samples), str(args.runs), str(output / (workload + ".txt"))], text=True)
        rows = []
        extras = {}
        for line in capture.splitlines():
            parts = line.split(",")
            if parts[0] == "sample":
                rows.append(dict(zip(["run", "sample", "normal_ns", "profiled_ns"], map(int, parts[1:]))))
            else:
                extras[parts[0]] = int(parts[1])
        summaries = {}
        for policy in ["normal", "profiled"]:
            values = sorted(row[policy + "_ns"] for row in rows)
            run_medians = [statistics.median(row[policy + "_ns"] for row in rows if row["run"] == run) for run in range(args.runs)]
            summaries[policy] = {"median_ns": statistics.median(values), "p95_ns": values[math.ceil(0.95 * len(values)) - 1], "maximum_ns": max(values), "run_medians_ns": run_medians}
        overhead = (summaries["profiled"]["median_ns"] / summaries["normal"]["median_ns"] - 1) * 100
        results["workloads"][workload] = {"samples": rows, "summary": summaries, "median_overhead_percent": overhead, **extras}
        print(f"{workload}: normal {summaries['normal']['median_ns']/1e6:.3f} ms, profiled {summaries['profiled']['median_ns']/1e6:.3f} ms, difference {overhead:+.2f}%", flush=True)
    (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    print(f"Results, source snapshot, compile commands, assembly and reports: {output}", flush=True)


if __name__ == "__main__":
    main()
