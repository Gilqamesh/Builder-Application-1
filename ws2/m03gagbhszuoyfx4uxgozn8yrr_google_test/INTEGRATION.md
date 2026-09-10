# GoogleTest integration

This module publishes GoogleTest and GoogleMock headers and libraries for C++ test
executables. [builder.cpp](builder.cpp) pins upstream **v1.15.0** and its SHA-256
checksum, downloads and extracts it during the source phase, and builds shared
libraries with CMake during the library phase. Upstream headers are published
phase artifacts, so a source checkout alone is not an include installation.

Use module-qualified includes in consuming modules:

```cpp
#include <m03gagbhszuoyfx4uxgozn8yrr_google_test/gtest/gtest.h>
#include <m03gagbhszuoyfx4uxgozn8yrr_google_test/gmock/gmock.h>
```

The interface phase also publishes unqualified `gtest/` and `gmock/` trees for
upstream headers' internal includes. Consumer includes should retain the module
prefix so Builder discovers the dependency. The module's library phase publishes
the installed `libgtest*` and `libgmock*` libraries, including entry-point variants.
Builder supplies dependency interfaces and libraries from the consumer's source
includes; consumers need no manual `-l` flags in their builder.

A consumer's `test/public_api.cpp` can register and run a test as follows:

```cpp
#include <m03gagbhszuoyfx4uxgozn8yrr_google_test/gtest/gtest.h>
#include <m03gagbhszuoyfx4uxgozn8yrr_google_test/gmock/gmock.h>

#include <vector>

TEST(sequence, order) {
    const std::vector<int> numbers { 1, 2, 3 };
    EXPECT_THAT(numbers, ::testing::ElementsAre(1, 2, 3));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv); // Also initializes GoogleTest.
    return RUN_ALL_TESTS(); // Nonzero if a test fails.
}
```

`TEST` registers the case; initialization followed by `RUN_ALL_TESTS()` actually
runs it. Keep one entry point: the example supplies `main()` explicitly. With a
manual link, use `gmock` and `gtest` for this example, or omit your own `main()` and
use the matching upstream `gmock_main` or `gtest_main` entry-point library.
Manual builds also need the interface install root on the include path, the
platform's thread link options, and runtime access to the installed shared
libraries. Do not copy upstream headers into the consuming module.

Builder automatically registers a consumer's `test/public_api.cpp` as the
`public_api` library validation. For an additional source, register it in the
consumer's existing library producer, after staging its library:

```cpp
phase->validate_library("google_test", { phase->source("test/google_test.cpp") });
```

Here `phase` is the borrowed
`m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t*` passed to
`phase__library`. This call registers validation for later compilation and
execution; it does not run the tests immediately. The
[build-phase public contract](../../ws0/m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h)
owns validation ordering, dependency linking, and phase lifetime rules.

Use the upstream v1.15.0 [GoogleTest primer](https://github.com/google/googletest/blob/v1.15.0/docs/primer.md)
and [GoogleMock introduction](https://github.com/google/googletest/blob/v1.15.0/docs/gmock_for_dummies.md)
for assertion, fixture, mock lifetime, and threading contracts. Local integration
does not change those contracts. The module's own `cli.cpp` only prints a greeting;
it is not a test runner.
