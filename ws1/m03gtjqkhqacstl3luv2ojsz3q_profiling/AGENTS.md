# `m03gtjqkhqacstl3luv2ojsz3q_profiling`

## Purpose

Measure synchronous code execution and retain the latest completed metric of each
producer-defined type. Producers own metric data, constructors, counter meanings,
formatters, and measurement boundaries. The profiler owns timing, replacement,
storage, and deferred reporting. See [the public contract](api.h).

## Invariants

- A profiler operates on one thread and owns growing storage. Capacity and allocation
  strategy are private. Creating a profiler or encountering a new metric type may
  allocate and fail; existing completed metrics survive allocation failure.
- Each active metric owns its application data and starting timestamp. Its stop()
  and destructor complete at most once, replacing that type's stored data, duration,
  and unwinding status together. The last completion wins, including overlapping
  measurements of the same type. Metrics have independent timing boundaries.
- Lookup and construction precede timing. Stop reads the clock before replacement;
  replacement performs no profiler-owned allocation, formatting, I/O, or locking.
  Metric construction, moves, and destruction are nonthrowing. Producers guarantee
  allocation-free metric operations and counter updates.
- Default metrics are inactive: they construct no application data and read no clock.
  Active metrics cannot be copied or moved. Stopping makes a metric inactive and
  ends mutable access to its data.
- Reads, reporting, attachment changes, and profiler destruction require no active
  metrics, including metrics undergoing construction or replacement. The profiler
  outlives active metrics and uses of its borrowed attachments. Borrowed data inside
  metrics remains valid through deferred use; retained pointers expire on replacement
  of that type or profiler destruction.

## Validation

Headless validation covers timing boundaries, replacement, independent completion,
heterogeneous types, growing storage, alignment, nonthrowing moves and destruction,
exception unwinding, inactive metrics, and reporting/allocation failures. Producers
validate their latest counters and identical application behavior with attached and
unattached instances of the same ordinary class.
