#include "helpers.h"

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

std::chrono::nanoseconds clock_now() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling
