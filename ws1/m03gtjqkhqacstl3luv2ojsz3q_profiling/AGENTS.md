# `m03gtjqkhqacstl3luv2ojsz3q_profiling`

## Purpose

Measure synchronous code execution. Producers own metric data, constructors,
counter meanings, formatters, and measurement boundaries. The profiler owns
storage, latest-data replacement, timing statistics, and deferred reporting.
See [the public contract](api.h) for enablement, lifetime, and type requirements.

## Validation

`test/public_api.cpp` exercises the public contract. Build it with
`PROFILING_TEST_CLOCK` for deterministic timing evidence and without the macro for
the production clock. Producers validate their counters and identical application
behavior with profiling enabled and disabled.
