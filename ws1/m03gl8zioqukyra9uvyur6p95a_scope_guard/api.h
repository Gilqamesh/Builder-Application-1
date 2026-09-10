#ifndef M03GL8ZIOQUKYRA9UVYUR6P95A_SCOPE_GUARD_API_H
# define M03GL8ZIOQUKYRA9UVYUR6P95A_SCOPE_GUARD_API_H

# include <functional>

namespace m03gl8zioqukyra9uvyur6p95a_scope_guard {

/**
 * @brief Runs an owned cleanup callback at scope exit unless explicitly released.
 *
 * Cleanup runs on normal exit and during exception unwinding, and must not throw.
 * An empty callback is accepted and does nothing. The guard owns the callable;
 * references and pointers captured by it remain borrowed and must stay valid
 * until cleanup or release. The guard cannot be copied or moved. Do not release
 * or destroy it concurrently.
 *
 * Construct the guard before acquiring the resource when creating its
 * std::function could throw. This example closes the file if initialization
 * throws, then cancels cleanup only after another owner has accepted the file.
 * @code{.cpp}
 * #include <m03gl8zioqukyra9uvyur6p95a_scope_guard/api.h>
 * #include <cstdio>
 * #include <memory>
 * #include <stdexcept>
 *
 * using file_t = std::unique_ptr<std::FILE, int (*)(std::FILE*)>;
 *
 * file_t make_file(void (*initialize)(std::FILE*)) {
 *     std::FILE* raw_file = nullptr;
 *     m03gl8zioqukyra9uvyur6p95a_scope_guard::scope_guard_t scope_guard([&] {
 *         if (raw_file != nullptr) {
 *             std::fclose(raw_file);
 *         }
 *     });
 *     raw_file = std::tmpfile();
 *     if (raw_file == nullptr) {
 *         throw std::runtime_error("failed to create temporary file");
 *     }
 *     initialize(raw_file); // Caller supplies a non-null callback; it may throw.
 *     file_t file(raw_file, &std::fclose); // Ownership transfer cannot throw.
 *     scope_guard.release();
 *     return file; // The returned owner now closes the file.
 * }
 * @endcode
 */
class scope_guard_t {
public:
    /** @brief Takes ownership of the cleanup callback without invoking it. */
    explicit scope_guard_t(std::function<void()> cleanup);

    /**
     * @brief Invokes a non-empty cleanup callback if the guard has not been released.
     *
     * Cleanup must not throw; an escaping exception calls std::terminate().
     */
    ~scope_guard_t() noexcept;

    scope_guard_t(const scope_guard_t& other) = delete;
    scope_guard_t& operator=(const scope_guard_t& other) = delete;
    scope_guard_t(scope_guard_t&& other) = delete;
    scope_guard_t& operator=(scope_guard_t&& other) = delete;

    /**
     * @brief Cancels cleanup without invoking the callback.
     *
     * Discards the stored callable and its captures immediately. Repeated calls
     * have no effect; the destructor will not invoke cleanup afterward.
     */
    void release() noexcept;

private:
    std::function<void()> m_cleanup;
};

} // namespace m03gl8zioqukyra9uvyur6p95a_scope_guard

#endif // M03GL8ZIOQUKYRA9UVYUR6P95A_SCOPE_GUARD_API_H
