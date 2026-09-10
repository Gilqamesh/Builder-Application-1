#ifndef M03GE9IJ43JYXY821PDA20JHWH_TYPESYSTEM_TYPESYSTEM_H
# define M03GE9IJ43JYXY821PDA20JHWH_TYPESYSTEM_TYPESYSTEM_H

# include <cmath>
# include <concepts>
# include <cstddef>
# include <memory>
# include <stdexcept>
# include <type_traits>
# include <unordered_map>
# include <utility>
# include <vector>

namespace m03ge9ij43jyxy821pda20jhwh_typesystem {

/**
 * @brief Registers value types and directed coercions to copy or convert live objects.
 *
 * Type IDs are zero-based registration positions local to this registry; do not
 * exchange IDs between independently populated registries or persist them as type
 * identities. Registration removes top-level cv-qualification, so `int` and
 * `const int` share an ID, while `int*` and `const int*` remain distinct.
 * Register every source, intermediate and destination type before registering
 * coercions or converting values. Same-type conversion uses copy assignment;
 * other conversions may compose registered procedures through intermediate values.
 * No particular route among alternatives is guaranteed. Coercion updates timing
 * state, so callers must synchronize concurrent use of the same registry.
 *
 * @code{.cpp}
 * #include <m03ge9ij43jyxy821pda20jhwh_typesystem/typesystem.h>
 *
 * #include <cassert>
 *
 * int main() {
 *     m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t typesystem;
 *     typesystem.register_type<int>();
 *     typesystem.register_type<double>();
 *     typesystem.register_coercion<int, double>(+[](int count) -> double {
 *         return static_cast<double>(count) / 2.0;
 *     });
 *     int count = 7;
 *     auto reader = typesystem.coerce(count);
 *     double half = reader; // Both count and typesystem are still alive.
 *     assert(half == 3.5);
 *     count = 8;
 *     half = reader; // Reads the current source each time.
 *     assert(half == 4.0);
 * }
 * @endcode
 */
class typesystem_t {
public:
    /**
     * @brief Borrows a source object and its registry for conversion to a caller-selected type.
     *
     * Keep both objects alive at the recorded addresses until the last conversion;
     * moving or replacing the registry must not change the meaning of type_id_from.
     * No source lifetime is extended: a reader saved from `coerce(7)` dangles after
     * that full expression. Prefer obtaining readers through coerce(from).
     */
    struct reader_t {
        /// @brief Registry that owns the source ID and coercion graph; borrowed.
        typesystem_t* self;
        /// @brief Live source of the registered type; borrowed, never copied into the reader.
        const void* from;
        /// @brief Source type ID in self, which must match the actual source object.
        int type_id_from;

        /// @brief Converts the current source to registered T, propagating coerce() failures.
        template <typename T>
        operator T() const {
            return self->coerce<T>(from, type_id_from);
        }
    };

    /**
     * @brief Invokes a type-erased assignment procedure with optional shared context ownership.
     *
     * For manually constructed coercions, caller defines the required source,
     * destination and context types. Keep borrowed context alive through every
     * invocation, or retain its owner in context_owner; setting the owner does not
     * set context automatically. Copying a coercion shares context_owner.
     */
    struct coercion_t {
        /// @brief Procedure receiving source, live destination and context, in that order.
        void (*caller)(const void*, void*, const void*) = nullptr;
        /// @brief Context passed unchanged to caller; may be null if caller supports it.
        const void* context = nullptr;
        /// @brief Optional ownership keeping the procedure's context alive.
        std::shared_ptr<const void> context_owner;

        /// @brief Tests only whether caller is non-null.
        constexpr operator bool() const {
            return caller != nullptr;
        }

        /// @brief Invokes a non-null caller without validating pointers; its exceptions propagate.
        void operator()(const void* from, void* to) const {
            caller(from, to, context);
        }
    };

public:
    /**
     * @brief Registers the unqualified form of a default-initializable, copy-assignable value type.
     * Re-registering a type is a no-op and preserves its ID. T must be a value
     * type, not a reference type; these requirements are checked at instantiation.
     */
    template <typename T>
    void register_type();

    /**
     * @brief Registers a directed value conversion and updates reachability automatically.
     * From and To must already be registered value types; the procedure accepts
     * From by value and returns To, which is assigned to a live destination.
     * No separate update_coercion_graph() call is needed for ordinary setup.
     * @throws std::invalid_argument If the procedure is null.
     * @throws std::runtime_error If a type is unregistered or this direct coercion already exists.
     */
    template <typename From, typename To>
    void register_coercion(To (*coercion_procedure)(From));

    /**
     * @brief Creates a reader borrowing from and this registry without converting yet.
     * @throws std::runtime_error If From is unregistered; the destination is checked when read.
     */
    template <typename From>
    reader_t coerce(const From& from);

    /**
     * @brief Returns a default-constructed To assigned from a borrowed source through coerce().
     * To must be default-initializable and registered. The pointer and ID must
     * describe the same live source object; the raw coerce() preconditions apply.
     */
    template <typename To>
    To coerce(const void* from, int id_from);

    /**
     * @brief Assigns a coerced value between live objects matching id_from and id_to.
     * The source is borrowed for the call and the destination must already be constructed.
     * Both objects must have the registered types and alignment: IDs do not verify
     * the pointed-to C++ types. Intermediate objects are constructed and destroyed
     * internally. Allocation, construction, assignment and procedure exceptions
     * propagate; destination rollback is not provided if assignment throws.
     * @throws std::invalid_argument If either object pointer is null.
     * @throws std::runtime_error If an ID is out of bounds or no coercion path exists.
     */
    void coerce(const void* from, int id_from, void* to, int id_to);

    /// @brief Returns an opaque token for unqualified T without requiring registration.
    template <typename T>
    const void* type_addr();

    /// @brief Returns the local ID for unqualified T, throwing std::runtime_error if unregistered.
    template <typename T>
    int type_id();

    /// @brief Looks up a type_addr() token, throwing std::runtime_error if unregistered here.
    int type_id(const void* addr);

    /// @brief Relaxes coercion routes using all currently stored costs.
    void update_coercion_graph();
    /**
     * @brief Relaxes routes through the specified edge using currently stored costs.
     * @throws std::runtime_error If either ID is out of bounds.
     */
    void update_coercion_graph(int id_from, int id_to);

    /// @brief Returns sizeof(T) in bytes without requiring registration.
    template <typename T>
    size_t sizeof_type();

    /// @brief Returns the registered size in bytes, throwing std::runtime_error for an invalid ID.
    size_t sizeof_type(int type_id);
    /// @brief Returns the registered alignment in bytes, throwing std::runtime_error for an invalid ID.
    size_t alignof_type(int type_id);
    /// @brief Returns the registered trivial-copyability trait, throwing std::runtime_error for an invalid ID.
    bool is_trivially_copyable(int type_id);

private:
    template <typename T>
    static const void* type_addr_impl();

    template <typename From, typename To>
    static void coercion_bridge(const void* from_raw, void* to_raw, const void* context);

    struct type_operations_t {
        size_t size;
        size_t alignment;
        bool trivially_copyable;
        void (*default_construct)(void*);
        void (*destroy)(void*);
        void (*copy_assign)(const void*, void*);
    };

private:
    std::unordered_map<const void*, int> m_addr_to_typeid;

    std::vector<std::vector<int>> m_type_parents;
    std::vector<std::vector<double>> m_type_distance;
    std::vector<std::vector<size_t>> m_type_calls;
    std::vector<std::vector<coercion_t>> m_coercions;
    std::vector<type_operations_t> m_type_operations;
};

template <typename T>
void typesystem_t::register_type() {
    using value_t = std::remove_cv_t<T>;
    static_assert(std::default_initializable<value_t>);
    static_assert(std::is_copy_assignable_v<value_t>);

    int old_size = (int) m_addr_to_typeid.size();
    const void* addr = type_addr<value_t>();
    if (!m_addr_to_typeid.emplace(addr, old_size).second) {
        return ;
    }

    for (int i = 0; i < old_size; ++i) {
        m_type_parents[i].emplace_back(-1);
        m_type_distance[i].emplace_back(INFINITY);
        m_type_calls[i].emplace_back(0);
        m_coercions[i].emplace_back(coercion_t{});
    }
    m_type_parents.emplace_back(std::vector<int>(old_size + 1, -1));
    m_type_distance.emplace_back(std::vector<double>(old_size + 1, INFINITY));
    m_type_calls.emplace_back(std::vector<size_t>(old_size + 1, 0));
    m_coercions.emplace_back(std::vector<coercion_t>(old_size + 1, coercion_t{}));

    m_type_parents[old_size][old_size] = old_size;
    m_type_distance[old_size][old_size] = 0.0;

    m_type_operations.emplace_back(type_operations_t {
        .size = sizeof(value_t),
        .alignment = alignof(value_t),
        .trivially_copyable = std::is_trivially_copyable_v<value_t>,
        .default_construct = [](void* storage) {
            std::construct_at(static_cast<value_t*>(storage));
        },
        .destroy = [](void* storage) {
            std::destroy_at(static_cast<value_t*>(storage));
        },
        .copy_assign = [](const void* from, void* to) {
            *static_cast<value_t*>(to) = *static_cast<const value_t*>(from);
        }
    });
}

template <typename From, typename To>
void typesystem_t::register_coercion(To (*coercion_procedure)(From)) {
    using procedure_t = To (*)(From);
    static_assert(std::is_copy_assignable_v<std::remove_cv_t<To>>);

    if (coercion_procedure == nullptr) {
        throw std::invalid_argument("coercion procedure must not be null");
    }

    int id_from = type_id<From>();
    int id_to = type_id<To>();
    if (m_coercions[id_from][id_to]) {
        throw std::runtime_error("coercion is already registered between types");
    }
    m_type_parents[id_from][id_to] = id_from;
    m_type_distance[id_from][id_to] = 0.0;
    m_type_calls[id_from][id_to] = 0;
    auto context_owner = std::make_shared<procedure_t>(coercion_procedure);
    m_coercions[id_from][id_to] = coercion_t {
        .caller = &coercion_bridge<From, To>,
        .context = context_owner.get(),
        .context_owner = std::move(context_owner)
    };
    update_coercion_graph(id_from, id_to);
}

template <typename From>
typesystem_t::reader_t typesystem_t::coerce(const From& from) {
    return reader_t {
        .self = this,
        .from = &from,
        .type_id_from = type_id<From>()
    };
}

template <typename To>
To typesystem_t::coerce(const void* from, int id_from) {
    static_assert(std::default_initializable<To>);

    int id_to = type_id<To>();
    To result{};
    coerce(from, id_from, &result, id_to);
    return result;
}

template <typename T>
const void* typesystem_t::type_addr() {
    using value_t = std::remove_cv_t<T>;
    return type_addr_impl<value_t>();
}

template <typename T>
const void* typesystem_t::type_addr_impl() {
    static int addr;
    return &addr;
}

template <typename T>
int typesystem_t::type_id() {
    return type_id(type_addr<std::remove_cv_t<T>>());
}

template <typename T>
size_t typesystem_t::sizeof_type() {
    return sizeof(std::remove_cv_t<T>);
}

template <typename From, typename To>
void typesystem_t::coercion_bridge(const void* from_raw, void* to_raw, const void* context) {
    using procedure_t = To (*)(From);
    const auto procedure = *static_cast<const procedure_t*>(context);
    *static_cast<To*>(to_raw) = procedure(*static_cast<const From*>(from_raw));
}

} // namespace m03ge9ij43jyxy821pda20jhwh_typesystem

#endif // M03GE9IJ43JYXY821PDA20JHWH_TYPESYSTEM_TYPESYSTEM_H
