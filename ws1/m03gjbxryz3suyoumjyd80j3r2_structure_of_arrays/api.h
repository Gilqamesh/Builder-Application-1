#ifndef M03GJBXRYZ3SUYOUMJYD80J3R2_STRUCTURE_OF_ARRAYS_API_H
# define M03GJBXRYZ3SUYOUMJYD80J3R2_STRUCTURE_OF_ARRAYS_API_H

# include <m03gjfvd6i5jzbmngb2ldoooza_type_erased_array/api.h>

# include <concepts>
# include <cstddef>
# include <format>
# include <span>
# include <tuple>
# include <type_traits>
# include <utility>
# include <vector>

namespace m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays {

/**
 * @brief Stores rows as separate typed columns in template-argument order.
 *
 * Requires at least one plain value type (no references or cv-qualified types).
 * Each successful push_back() adds the corresponding argument to each column;
 * the same index across columns identifies one row. Copies own independent columns.
 *
 * @code{.cpp}
 * #include <m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays/api.h>
 *
 * #include <cassert>
 * #include <tuple>
 * #include <utility>
 *
 * int main() {
 *     using namespace m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays;
 *     structure_of_arrays_t<int, float> structure_of_arrays;
 *     structure_of_arrays.push_back(7, 1.5F);
 *     structure_of_arrays.push_back(9, 2.5F);
 *     structure_of_arrays.push_back(11, 3.5F);
 *     {
 *         const auto& columns = structure_of_arrays.data(); // Borrow the tuple.
 *         assert(std::get<0>(columns)[1] == 9);    // int column: 7, 9, 11
 *         assert(std::get<1>(columns)[1] == 2.5F); // float column: 1.5, 2.5, 3.5
 *     }
 *     erased_structure_of_arrays_t erased_structure_of_arrays(std::move(structure_of_arrays));
 *     assert(erased_structure_of_arrays.size() == 2); // Columns, not the three rows.
 *     assert(erased_structure_of_arrays[0].element_count() == 3);
 *     assert(erased_structure_of_arrays[1].read<float>(1) == 2.5F);
 *     auto owned_columns = std::move(erased_structure_of_arrays).data();
 *     assert(owned_columns[0].read<int>(2) == 11); // Owned independently of the wrapper.
 * }
 * @endcode
 */
template <typename... Ts>
class structure_of_arrays_t {
    static_assert(0 < sizeof...(Ts));
    static_assert((std::same_as<Ts, std::remove_cvref_t<Ts>> && ...), "structure_of_arrays_t only supports plain value types (no references, no const, no volatile).");
public:
    /** @brief Holds one vector per column, in the order of Ts. */
    using data_t = std::tuple<std::vector<Ts>...>;

public:
    /**
     * @brief Appends one row by moving each by-value argument into its column.
     *
     * If a column insertion throws, additions to earlier columns are removed and
     * the exception is rethrown. Each vector's exception guarantees still depend
     * on its element type. Growth may invalidate column element references even
     * when a later insertion fails and the earlier additions are removed.
     */
    void push_back(Ts... values);

    /**
     * @brief Borrows the read-only tuple of columns while this object remains alive.
     *
     * The tuple reference remains attached to this object; push_back() can invalidate
     * references, pointers and iterators into its vectors. Assignment or moving the
     * columns out changes what the tuple contains. Keep the owner alive when borrowing,
     * including when this overload is called on a const temporary.
     */
    const data_t& data() const&;

    /**
     * @brief Transfers the column vectors into an owning tuple returned by value.
     *
     * Call as std::move(structure_of_arrays).data(); the source columns are left moved-from.
     */
    data_t data()&&;

private:
    std::tuple<std::vector<Ts>...> m_data;
};

/**
 * @brief Owns ordered byte-backed columns copied from a typed structure of arrays.
 *
 * Column indices preserve the source template-argument order. Each column records
 * element width and row count, not C++ type identity; callers must retain the schema.
 * Typed reads and references follow the contract of
 * m03gjfvd6i5jzbmngb2ldoooza_type_erased_array::type_erased_array_t.
 */
class erased_structure_of_arrays_t {
public:
    /** @brief Holds the owned erased columns in source order. */
    using data_t = std::vector<m03gjfvd6i5jzbmngb2ldoooza_type_erased_array::type_erased_array_t>;

public:
    /** @brief Constructs an empty collection with zero columns. */
    erased_structure_of_arrays_t() = default;

    /**
     * @brief Copies each typed column's object representations into independent byte storage.
     *
     * Pass an lvalue to copy the source or std::move(source) to consume its columns.
     * Each Ts must meet type_erased_array_t's vector-constructor requirements:
     * trivially copyable, not bool, and alignment no greater than std::max_align_t.
     * Allocation and source-copy exceptions propagate.
     */
    template <typename... Ts>
    explicit erased_structure_of_arrays_t(structure_of_arrays_t<Ts...> soa);

    /**
     * @brief Borrows the read-only vector of columns while this object remains alive.
     *
     * Assignment or moving the columns out changes the vector's contents; reacquire
     * column references afterward. A borrowed reference does not extend owner lifetime.
     */
    const data_t& data() const&;

    /**
     * @brief Transfers the erased columns into an owning vector returned by value.
     *
     * Call on a non-const rvalue; the source vector is left moved-from.
     */
    data_t data() &&;

    /** @brief Returns the number of columns, not rows or bytes. */
    size_t size() const noexcept;

    /**
     * @brief Borrows the read-only column at a zero-based schema index.
     *
     * @pre index < size(); column bounds are not checked.
     * The reference borrows this object's column storage and follows data()'s lifetime rules.
     */
    const m03gjfvd6i5jzbmngb2ldoooza_type_erased_array::type_erased_array_t& operator[](size_t index) const&;

private:
    data_t m_data;
};

} // namespace m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays

namespace std {

template <typename... Ts>
struct formatter<m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::structure_of_arrays_t<Ts...>>;

template <>
struct formatter<m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::erased_structure_of_arrays_t>;

} // namespace std

namespace m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays {

template <typename... Ts>
void structure_of_arrays_t<Ts...>::push_back(Ts... values) {
    std::size_t pushed_count = 0;
    try {
        std::apply([&](auto&... vector) {
            ((vector.push_back(std::move(values)), ++pushed_count), ...);
        }, m_data);
    } catch (...) {
        std::size_t index = 0;
        std::apply([&](auto&... vector) {
            ((index++ < pushed_count ? vector.pop_back() : void()), ...);
        }, m_data);
        throw;
    }
}

template <typename... Ts>
const structure_of_arrays_t<Ts...>::data_t& structure_of_arrays_t<Ts...>::data() const& {
    return m_data;
}

template <typename... Ts>
typename structure_of_arrays_t<Ts...>::data_t structure_of_arrays_t<Ts...>::data() && {
    return std::move(m_data);
}


template <typename... Ts>
erased_structure_of_arrays_t::erased_structure_of_arrays_t(structure_of_arrays_t<Ts...> soa) {
    static_assert((std::is_trivially_copyable_v<Ts> && ...), "erased_structure_of_arrays_t does not support non-trivially copyable types.");

    m_data.reserve(sizeof...(Ts));
    std::apply([this](auto&&... stream) {
        (m_data.emplace_back(std::move(stream)), ...);
    }, std::move(soa.data()));
}

} // namespace m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays

namespace std {

template <typename... Ts>
struct formatter<m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::structure_of_arrays_t<Ts...>> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::structure_of_arrays_t<Ts...>& soa, auto& ctx) const {
        auto out = ctx.out();

        out = format_to(out, "{{ ");

        size_t stream_index = 0;
        bool first_stream = true;

        apply(
            [&](const auto&... streams) {
                (
                    [&] {
                        if (!first_stream) {
                            out = format_to(out, ", ");
                        }
                        first_stream = false;

                        out = format_to(out, "[{}]: {{ element_size: {}, size: {}, data: [ ", stream_index++, sizeof(typename std::remove_cvref_t<decltype(streams)>::value_type), streams.size());

                        bool first_value = true;

                        for (const auto& value : streams) {
                            if (!first_value) {
                                out = format_to(out, ", ");
                            }
                            first_value = false;

                            out = format_to(out, "{}", value);
                        }

                        out = format_to(out, " ] }}");
                    }(),
                    ...
                );
            },
            soa.data()
        );

        out = format_to(out, " }}");

        return out;
    }
};

template <>
struct formatter<m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::erased_structure_of_arrays_t> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::erased_structure_of_arrays_t& esoa, auto& ctx) const {
        auto out = ctx.out();

        out = format_to(out, "{{ ");

        for (size_t i = 0; i < esoa.data().size(); ++i) {
            if (0 < i) {
                out = format_to(out, ", ");
            }
            out = format_to(out, "[{}]: {{ {} }}", i, esoa.data()[i]);
        }

        out = format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GJBXRYZ3SUYOUMJYD80J3R2_STRUCTURE_OF_ARRAYS_API_H
