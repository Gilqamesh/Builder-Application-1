# `m03gtjqkhqacstl3luv2ojsz3q_profiling`

## Purpose

Collect synchronous nested timings and producer-defined performance counters,
then traverse the captured hierarchy for deferred reporting.

Applications own capture storage, capture periods, and report destinations.
Producers own regions, payload types, counter meanings, and payload formatters.
This module owns region registration, timing, nesting, collection, and generic
report traversal. See [the public contract](api.h).

## Invariants

- Region setup precedes capture. Enabled profilers own immutable registered
  metadata through reporting; capture resets preserve registration.
- Each profiler records on one thread with synchronous, strictly nested scopes.
  Collection borrows fixed-capacity application storage and performs no allocation,
  formatting, I/O, or locking. Producer payload operations must preserve this rule.
- Overflow omits a scope and its descendants while preserving retained hierarchy
  and reporting omissions. Exceptional unwinding preserves partial observations.
- A `void` report policy disables profiling at compilation, including payload
  construction and formatter requirements. Producers discard counter work through
  dependent compile-time branches.

## Validation

Headless public validation covers lifecycle, nesting, overflow, reporting, and
disabled policies. Producers validate their own counter meanings and unchanged
observable behavior with both policies in the same executable.
