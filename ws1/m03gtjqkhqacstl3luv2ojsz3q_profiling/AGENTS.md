# `m03gtjqkhqacstl3luv2ojsz3q_profiling`

## Purpose

Measure synchronous code execution. Producers own metric data, constructors,
counter meanings, formatters, and measurement boundaries. The profiler owns
persistent storage, timing statistics, and deferred reporting. Applications own
profilers; measured metrics borrow them and explicitly create child metrics.
Data accumulates per parent/type path, with children reported in first-use order.
See [the public contract](api.h) for enablement, lifetime, and type requirements.

## Validation

`test/public_api.cpp` exercises the public contract. Build it with
`PROFILING_TEST_CLOCK` for deterministic timing evidence and without the macro for
the production clock. Producers validate their counters and identical application
behavior with profiling enabled and disabled.
