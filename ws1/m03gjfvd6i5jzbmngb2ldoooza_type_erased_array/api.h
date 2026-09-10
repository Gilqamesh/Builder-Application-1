#ifndef M03GJFVD6I5JZBMNGB2LDOOOZA_TYPE_ERASED_ARRAY_API_H
# define M03GJFVD6I5JZBMNGB2LDOOOZA_TYPE_ERASED_ARRAY_API_H

# include <array>
# include <bit>
# include <concepts>
# include <cstddef>
# include <cstring>
# include <format>
# include <memory>
# include <span>
# include <stdexcept>
# include <string_view>
# include <type_traits>
# include <vector>

# include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>

namespace m03gjfvd6i5jzbmngb2ldoooza_type_erased_array {

/**
 * @brief Owns a sequence of equal-sized object representations with caller-supplied element types.
 *
 * Storage records element size, not C++ type identity. Typed operations require a
 * trivially copyable T other than bool, with alignof(T) <= alignof(std::max_align_t).
 * Runtime checks compare sizeof(T) and, for access, the element index; they do not
 * prove that a same-sized type is appropriate for the stored bytes.
 *
 * Elements are contiguous native object representations, including any padding,
 * in insertion order. This is not a portable serialization format. Copies own
 * independent byte storage; resources referenced by the stored representations are
 * not acquired or managed. Use `read<T>()` for an independent value; `operator[]<T>()`
 * additionally requires a suitably aligned live T in the storage, as documented below.
 *
 * @code{.cpp}
 * #include <m03gjfvd6i5jzbmngb2ldoooza_type_erased_array/api.h>
 *
 * #include <cassert>
 * #include <vector>
 *
 * int main() {
 *     using m03gjfvd6i5jzbmngb2ldoooza_type_erased_array::type_erased_array_t;
 *     type_erased_array_t type_erased_array(std::vector<int> { 10, 20 });
 *     const int first = type_erased_array.read<int>(0); // Owns a copy of 10.
 *     {
 *         const auto bytes = type_erased_array.data(); // Borrows writable bytes.
 *         assert(bytes.size() == 2 * sizeof(int));
 *     } // Stop using the view before possible storage growth.
 *     type_erased_array.push_back(30);
 *     assert(type_erased_array.read<int>(2) == 30);
 *     type_erased_array.clear();
 *     assert(first == 10); // Copied reads survive growth and clearing.
 *     assert(type_erased_array.element_size() == sizeof(int));
 *     type_erased_array.push_back(40); // clear() retains the established width.
 * }
 * @endcode
 */
class type_erased_array_t {
public:
    /** @brief Constructs an empty array with no established element size (zero). */
    type_erased_array_t();

    /**
     * @brief Copies a vector's element representations into independently owned bytes.
     *
     * Establishes sizeof(T) even for an empty vector. The vector parameter is passed
     * by value; its allocation is not reused as typed storage in this array.
     */
    template <typename T>
    requires (std::is_trivially_copyable_v<T> && !std::same_as<T, bool> && alignof(T) <= alignof(std::max_align_t))
    explicit type_erased_array_t(std::vector<T> values);

    /**
     * @brief Borrows all element representations as read-only bytes.
     *
     * Views and typed references borrow the underlying byte_stream_t storage:
     * growth that reallocates, clear(), non-self assignment and destruction invalidate
     * them. Moves transfer that storage to the destination. Reacquire views after growth;
     * a span's size does not track appended elements.
     * @see m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t
     */
    std::span<const std::byte> data() const& noexcept;
    /**
     * @brief Borrows writable bytes without changing the element size or count.
     *
     * Uses the read-only data() view's lifetime rules. Callers must preserve valid
     * object representations for subsequent typed reads or reference access.
     */
    std::span<std::byte> data() & noexcept;

    std::span<const std::byte> data() const && = delete;
    std::span<std::byte> data() && = delete;

    /** @brief Returns the number of stored elements, or zero before a width is established. */
    size_t element_count() const noexcept;
    /** @brief Returns the established element width in bytes, or zero if none is established. */
    size_t element_size() const noexcept;
    /** @brief Returns the total byte count, equal to element_count() * element_size(). */
    size_t byte_size() const noexcept;

    /** @brief Removes all elements and invalidates views while retaining the element size. */
    void clear();

    /**
     * @brief Copies one element out after checking its stored size and index.
     *
     * The result owns its value and remains independent of later storage changes.
     * @pre The selected bytes form a valid object representation of T. This operation
     * does not require a live T in the byte storage and does not check type identity.
     * @throws std::invalid_argument If sizeof(T) differs from element_size(), including
     * an unconfigured empty array whose element size is zero.
     * @throws std::out_of_range If the sizes match but index >= element_count().
     */
    template <typename T>
    requires (std::is_trivially_copyable_v<T> && !std::same_as<T, bool> && alignof(T) <= alignof(std::max_align_t))
    T read(size_t index) const;

    /**
     * @brief Borrows a mutable T reference after checking element size and index.
     *
     * @pre The selected address denotes a suitably aligned live T that can be accessed
     * through T; matching sizeof(T) alone is insufficient. The implementation casts a
     * byte address without establishing or checking T's lifetime. Prefer `read<T>()`
     * when only an object representation is available.
     * Changes through the reference affect the stored bytes. References follow data()'s
     * invalidation rules and do not extend storage lifetime.
     * @throws std::invalid_argument If sizeof(T) differs from element_size().
     * @throws std::out_of_range If the sizes match but index >= element_count().
     */
    template <typename T>
    requires (std::is_trivially_copyable_v<T> && !std::same_as<T, bool> && alignof(T) <= alignof(std::max_align_t))
    T& operator[](size_t index) &;

    /**
     * @brief Borrows a read-only T reference with the mutable overload's preconditions and checks.
     *
     * A const reference also requires a suitably aligned live T and follows data()'s
     * invalidation rules. Do not retain a reference obtained from a temporary array.
     */
    template <typename T>
    requires (std::is_trivially_copyable_v<T> && !std::same_as<T, bool> && alignof(T) <= alignof(std::max_align_t))
    const T& operator[](size_t index) const&;

    /**
     * @brief Appends a copy of T's representation, establishing an element size if it is zero.
     *
     * Only sizeof(T) is compared with an existing size; same-sized unrelated types
     * are not rejected. The caller retains responsibility for the type used to read
     * each element. A value borrowed from this array can be appended because its bytes
     * are copied before storage growth. Growth may invalidate views and references.
     * @throws std::invalid_argument If a nonzero element_size() differs from sizeof(T).
     * Allocation exceptions propagate; a first append that fails during allocation
     * can still leave sizeof(T) established as the element size.
     */
    template <typename T>
    requires (std::is_trivially_copyable_v<T> && !std::same_as<T, bool> && alignof(T) <= alignof(std::max_align_t))
    void push_back(const T& value);

private:
    size_t checked_byte_offset(size_t expected_element_size, size_t index, std::string_view operation) const;

    m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t m_data;
    size_t m_element_size;
};

} // namespace m03gjfvd6i5jzbmngb2ldoooza_type_erased_array

namespace std {

template <>
struct formatter<m03gjfvd6i5jzbmngb2ldoooza_type_erased_array::type_erased_array_t>;

} // namespace std

namespace m03gjfvd6i5jzbmngb2ldoooza_type_erased_array {

template <typename T>
requires (std::is_trivially_copyable_v<T> && !std::same_as<T, bool> && alignof(T) <= alignof(std::max_align_t))
type_erased_array_t::type_erased_array_t(std::vector<T> values):
    m_data(std::as_bytes(std::span<const T>(values.begin(), values.end()))),
    m_element_size(sizeof(T))
{
}

template <typename T>
requires (std::is_trivially_copyable_v<T> && !std::same_as<T, bool> && alignof(T) <= alignof(std::max_align_t))
T type_erased_array_t::read(size_t index) const {
    const size_t offset = checked_byte_offset(sizeof(T), index, "type_erased_array_t::read");
    std::array<std::byte, sizeof(T)> bytes;
    std::memcpy(bytes.data(), m_data.bytes().data() + offset, bytes.size());
    return std::bit_cast<T>(bytes);
}

template <typename T>
requires (std::is_trivially_copyable_v<T> && !std::same_as<T, bool> && alignof(T) <= alignof(std::max_align_t))
T& type_erased_array_t::operator[](size_t index) & {
    const size_t offset = checked_byte_offset(sizeof(T), index, "type_erased_array_t::operator[]");
    return *reinterpret_cast<T*>(m_data.bytes().data() + offset);
}

template <typename T>
requires (std::is_trivially_copyable_v<T> && !std::same_as<T, bool> && alignof(T) <= alignof(std::max_align_t))
const T& type_erased_array_t::operator[](size_t index) const& {
    const size_t offset = checked_byte_offset(sizeof(T), index, "type_erased_array_t::operator[]");
    return *reinterpret_cast<const T*>(m_data.bytes().data() + offset);
}

template <typename T>
requires (std::is_trivially_copyable_v<T> && !std::same_as<T, bool> && alignof(T) <= alignof(std::max_align_t))
void type_erased_array_t::push_back(const T& value) {
    if (m_element_size == 0) {
        m_element_size = sizeof(T);
    } else if (m_element_size != sizeof(T)) {
        throw std::invalid_argument(std::format("type_erased_array_t::push_back: type mismatch, expected element size {}, got {}", m_element_size, sizeof(T)));
    }

    m_data.append(std::span<const std::byte>(reinterpret_cast<const std::byte*>(&value), sizeof(T)));
}

} // namespace m03gjfvd6i5jzbmngb2ldoooza_type_erased_array

namespace std {

template <>
struct formatter<m03gjfvd6i5jzbmngb2ldoooza_type_erased_array::type_erased_array_t> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gjfvd6i5jzbmngb2ldoooza_type_erased_array::type_erased_array_t& tea, auto& ctx) const {
        auto out = ctx.out();

        out = format_to(out, "element_size: {}, element_count: {}", tea.element_size(), tea.element_count());

        if (0 < tea.element_count()) {
            out = format_to(out, ", data: 0x");
            for (const auto& byte : tea.data()) {
                out = format_to(out, "{:02x}", std::to_integer<unsigned int>(byte));
            }
        }

        return out;
    }
};

} // namespace std

#endif // M03GJFVD6I5JZBMNGB2LDOOOZA_TYPE_ERASED_ARRAY_API_H
