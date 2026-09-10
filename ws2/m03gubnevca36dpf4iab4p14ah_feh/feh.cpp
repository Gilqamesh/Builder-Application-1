#include "feh.h"

#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <format>
#include <stdexcept>

#ifndef M03GUBNEVCA36DPF4IAB4P14AH_FEH_FEH_PATH
# error M03GUBNEVCA36DPF4IAB4P14AH_FEH_FEH_PATH must be defined by the owning builder
#endif

namespace m03gubnevca36dpf4iab4p14ah_feh {

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t feh_path() {
    const auto result = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(M03GUBNEVCA36DPF4IAB4P14AH_FEH_FEH_PATH);
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(result) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(result)) {
        throw std::runtime_error(std::format("m03gubnevca36dpf4iab4p14ah_feh::view: host tool '{}' does not exist or is not a regular file", result));
    }

    return result;
}

static void throw_process_error(int process_result) {
    if (0 < process_result) {
        throw std::runtime_error(std::format("m03gubnevca36dpf4iab4p14ah_feh::view: command failed with exit code {}", process_result));
    } else if (process_result < 0) {
        throw std::runtime_error(std::format("m03gubnevca36dpf4iab4p14ah_feh::view: command terminated by signal {}", -process_result));
    }
}

void view(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) {
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(path)) {
        throw std::runtime_error(std::format("m03gubnevca36dpf4iab4p14ah_feh::view: path '{}' does not exist", path));
    }

    throw_process_error(m03gagbhsvr0m5w15urj0o291m_process::create_and_wait(m03gagbhsvr0m5w15urj0o291m_process::command_t {
        {
            feh_path().string(),
            path.string()
        }
    }));
}

} // namespace m03gubnevca36dpf4iab4p14ah_feh
