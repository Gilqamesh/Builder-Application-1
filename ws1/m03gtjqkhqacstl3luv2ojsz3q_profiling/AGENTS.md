# `m03gtjqkhqacstl3luv2ojsz3q_profiling`

## Purpose

Collect synchronous nested timings and producer-defined metrics, then traverse
retained heterogeneous payloads for deferred reporting.

Applications own capture storage, capture periods, attachments, and report
destinations. Producers own payload types, constructors, counter meanings,
formatters, and measurement boundaries. This module owns timing, nesting, retained
payload lifetimes, and generic reporting. See [the public contract](api.h).

## Invariants

- The ordinary collector borrows fixed-capacity application byte storage. Ordinary
  producer contexts borrow the collector; default contexts are unattached.
- Each collector records on one thread with synchronous, strictly nested measurements.
  Collection performs no profiler-owned allocation, formatting, I/O, or locking.
  Producer payload operations and counter updates preserve the allocation rule.
- Typed measurements construct payloads directly in aligned, stable storage. Closure
  retains payloads through reporting; reset and destruction release them.
- Exhaustion suppresses an omitted measurement and its descendants while preserving
  retained hierarchy and reporting every omission. False suppressed handles still
  close their bookkeeping. Exceptional unwinding preserves partial observations.
- Unattached contexts construct no payloads and read no clock. Formatter and
  nonthrowing construction/destruction requirements apply at compilation even
  when recording will be unattached. Caller argument expressions still evaluate.
- Reads, reports, resets, and attachment changes occur without active measurements.
  Reset preserves contexts and invalidates retained views. Payload borrowing lasts
  through deferred use; the collector and storage outlive active handles.

## Validation

Headless validation covers lifecycle, heterogeneous types, deferred formatting and
payload destruction, alignment, nesting, exhaustion, attachment changes, and
inactive contexts. Producers validate their counters and unchanged observable
behavior with attached and unattached instances of the same ordinary class.
