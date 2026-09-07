#ifndef M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H
# define M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H

# include <chrono>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

// Internal clock seam; production uses steady_clock and deterministic validation
// can link the collector against a nonthrowing test clock instead.
std::chrono::nanoseconds clock_now() noexcept;

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

#endif // M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H
