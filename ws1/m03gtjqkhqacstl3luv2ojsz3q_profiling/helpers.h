#ifndef M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H
# define M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H

# include <chrono>
# include <ratio>
# include <string>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

// Internal clock seam; production uses steady_clock and deterministic validation
// supplies a nonthrowing test clock when PROFILING_TEST_CLOCK is defined.
std::chrono::nanoseconds clock_now() noexcept;

// Wide input supports the complete reporting scale without changing clock storage.
std::string format_elapsed(std::chrono::duration<long double, std::nano> elapsed);

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

#endif // M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H
