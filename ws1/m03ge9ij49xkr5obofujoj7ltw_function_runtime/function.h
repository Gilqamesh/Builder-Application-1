#ifndef M03GE9IJ49XKR5OBOFUJOJ7LTW_FUNCTION_RUNTIME_FUNCTION_H
# define M03GE9IJ49XKR5OBOFUJOJ7LTW_FUNCTION_RUNTIME_FUNCTION_H

# include <cstdint>
# include <format>
# include <stdexcept>
# include <string>
# include <vector>
# include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
# include <m03ge9ij43jyxy821pda20jhwh_typesystem/typesystem.h>

namespace m03ge9ij49xkr5obofujoj7ltw_function_runtime {

/**
 * @brief Executes a callback over mutable typed ports and a directed graph of borrowed nodes.
 *
 * The node owns its IR copy and port byte buffers, and borrows its typesystem,
 * parent, connection targets and children. Keep the typesystem alive at the same
 * address for the node's lifetime. Connected nodes must use the same registry:
 * sends copy type IDs without translating between registries. Keep a target and
 * its addressed port alive until every incoming link is disconnected or its
 * source is destroyed. Destruction clears this node's outgoing links only; it
 * neither repairs incoming links nor deletes children or clears their parents.
 * Manage child lifetimes separately, including children returned during expand().
 *
 * Ports are zero-based uint8_t indices. Resize arguments() before use and keep
 * at most 256 ports; the vector does not enforce this representational limit.
 * Use trivially copyable payloads such as int, bool or non-owning pointers:
 * the runtime does not construct or destroy nontrivial objects in its buffers,
 * and provides no storage guarantee for over-aligned types. Pointer payloads
 * borrow their pointees. Type registration and conversion rules belong to
 * m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t.
 *
 * Calls and propagation run synchronously on the caller's thread. There is no
 * scheduling, input-readiness check or cycle detection. Callbacks must terminate
 * propagation and preserve nodes and addressed ports while a send is active.
 * Exceptions propagate after any writes/connections already made; there is no
 * graph-wide rollback. Synchronize access to the graph and shared typesystem.
 *
 * @code{.cpp}
 * #include <m03ge9ij49xkr5obofujoj7ltw_function_runtime/function.h>
 * #include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
 * #include <m03ge9ij43jyxy821pda20jhwh_typesystem/typesystem.h>
 *
 * #include <cassert>
 * #include <cstdint>
 *
 * int main() {
 *     using m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t;
 *     using m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t;
 *     m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t typesystem;
 *     function_t target(typesystem, function_ir_t {}, +[](function_t& function, std::uint8_t index) {
 *         assert(index == 0);
 *         const int input = function.read(index);
 *         assert(input == 42);
 *     });
 *     function_t source(typesystem, function_ir_t {}, +[](function_t&, std::uint8_t) {});
 *     target.arguments().resize(1);
 *     source.arguments().resize(1);
 *     source.write(0, 42);
 *     source.connect(&target, 0, 0); // Immediately delivers stored data and calls target.
 *     source.disconnect(0); // Clears only source's outgoing link; target keeps 42.
 * } // Sources are destroyed before targets, then the shared typesystem.
 * @endcode
 */
class function_t {
public:
    /**
     * @brief Stores one port's name, byte payload and optional outgoing connection.
     *
     * Public fields are mutable; direct changes bypass runtime checks. A nonempty
     * payload's size must match its type ID in the node's typesystem. See function_t
     * for byte-storage restrictions and borrowed connection lifetimes.
     */
    struct argument_t {
        /// @brief Creates an unnamed, empty, disconnected port with type ID -1.
        argument_t();

        /// @brief Borrowed destination node, or nullptr for an unconnected port.
        function_t* m_connection;
        /// @brief Destination port index, ignored when m_connection is null.
        uint8_t m_connection_argument_index;
        /// @brief Caller-controlled display name with no effect on routing.
        std::string m_name;
        /// @brief Type ID in the owning node's registry, or -1 for no data.
        int m_data_type_id;
        /// @brief Owned payload bytes; see function_t for permitted payload storage.
        std::vector<uint8_t> m_data;
    };

    /**
     * @brief Borrows a node and port index to read its current data on conversion.
     *
     * This is not a snapshot. Keep self alive until conversion; mutations to the
     * indexed port affect subsequent reads. No pointer into the port vector is kept.
     */
    struct reader_t {
        /// @brief Borrowed node, which must be non-null when converting.
        function_t* self;
        /// @brief Port selected when read() created this reader.
        uint8_t index;

        /// @brief Returns the current payload coerced to registered T, propagating read failures.
        template <typename T>
        operator T() {
            return self->read<T>(index);
        }
    };

public:
    /**
     * @brief Stores an IR copy and a required callback without creating ports or expanding children.
     *
     * Borrows typesystem; the caller configures arguments() and layout separately.
     * IR rectangle fields do not initialize this node's runtime rectangle, which
     * starts at zero with unfinalized dimensions. The IR itself is not validated.
     * @throws std::invalid_argument If function_call is null.
     */
    function_t(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem, m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t function_ir, void (*function_call)(function_t&, uint8_t));

    /// @brief Clears outgoing links without deleting borrowed nodes or repairing incoming links.
    virtual ~function_t();

    function_t(const function_t& other) = delete;
    function_t& operator=(const function_t& other) = delete;
    function_t(function_t&& other) = delete;
    function_t& operator=(function_t&& other) = delete;

    /// @brief Returns the borrowed parent pointer, initially nullptr.
    function_t* parent();
    /**
     * @brief Records a borrowed parent pointer, accepting nullptr to clear it.
     *
     * Does not update either node's children or connections. Keep a non-null parent
     * alive while using the pointer; expand() does not set this field automatically.
     */
    void parent(function_t* parent);

    /**
     * @brief Returns this node's mutable, owned IR description.
     *
     * Edits do not rebuild an expanded graph or update the runtime rectangle.
     * References into its vectors follow std::vector invalidation rules.
     */
    m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t& function_ir();
    /// @brief Exposes the callback slot; replacements must remain non-null and obey call().
    void (*&function_call())(function_t&, uint8_t);

    /// @brief Replaces a port name, throwing std::runtime_error for an unavailable index.
    void argument_name(uint8_t argument_index, std::string name);
    /**
     * @brief Borrows a port name, throwing std::runtime_error for an unavailable index.
     *
     * Port erasure, vector reallocation or node destruction invalidates the reference.
     */
    const std::string& argument_name(uint8_t argument_index);

    /**
     * @brief Replaces one outgoing link and immediately sends any stored source data.
     *
     * Connects self_argument_index to other_argument_index of borrowed other; no
     * reverse link is created. Ports must already exist and use the same typesystem.
     * @throws std::invalid_argument If other is null.
     * @throws std::runtime_error If either port is unavailable or the stored byte count is invalid.
     *
     * Allocation and callback exceptions also propagate; link installation is
     * not rolled back on send failure.
     */
    void connect(function_t* other, uint8_t other_argument_index, uint8_t self_argument_index);

    /// @brief Tests for an outgoing link, throwing std::runtime_error for an unavailable port.
    bool is_connected(uint8_t argument_index);

    /// @brief Borrows the outgoing target or returns nullptr; an unavailable port throws std::runtime_error.
    function_t* connection(uint8_t argument_index);

    /**
     * @brief Clears only this port's outgoing link, leaving both payloads untouched.
     *
     * An unconnected port is a no-op; an unavailable index throws std::runtime_error.
     */
    void disconnect(uint8_t argument_index);

    /**
     * @brief Invokes the configured callback with this node and the triggering index unchanged.
     *
     * The callback defines accepted indices; call() performs no port bounds check
     * and does not itself expand the IR. A connected send supplies the destination
     * port index. Keep function_call() non-null; callback exceptions propagate.
     */
    void call(uint8_t caller_argument_index);

    /**
     * @brief Creates a borrowing reader whose conversion coerces the selected port to T.
     *
     * Validation occurs on conversion, not reader creation. The conversion throws
     * std::runtime_error for an unavailable port, empty data or a mismatched byte
     * count, and propagates typesystem failures (unregistered T, missing coercion,
     * or an exception from a coercion procedure).
     */
    reader_t read(uint8_t index);

    /**
     * @brief Registers T, copies its representation to a port and sends it to any target.
     *
     * Does not call this node itself. See function_t for byte-storage restrictions
     * and the raw write() overload for validation and propagation failures.
     */
    template <typename T>
    void write(uint8_t argument_index, T data);

    /**
     * @brief Copies registered-type bytes from borrowed data into a port and sends them.
     *
     * data must point to at least sizeof_type(data_type_id) readable bytes of that
     * type for the duration of the copy, without overlapping destination storage.
     * A type ID of -1 is a no-op even for an unavailable port or null data.
     * @throws std::invalid_argument If data is null for an ID other than -1.
     * @throws std::runtime_error If the port or type ID is unavailable.
     *
     * Allocation and send() failures propagate; stored data is not rolled back.
     */
    void write(uint8_t argument_index, void* data, int data_type_id);

    /**
     * @brief Copies stored data to the outgoing target and calls its destination port synchronously.
     *
     * Does nothing if the source is empty or unconnected. Throws std::runtime_error
     * for an unavailable source port or mismatched byte count; registry and callback
     * failures propagate. The target port must still exist at its recorded index.
     */
    void send(uint8_t argument_index);

    /**
     * @brief Empties this port and its immediate outgoing target without invoking callbacks.
     *
     * Does not propagate beyond that target or remove links. An unavailable source
     * index throws std::runtime_error; a connected destination port must still exist.
     */
    void clear(uint8_t argument_index);

    /**
     * @brief Writes the source port's stored type and bytes to a distinct destination port.
     *
     * An empty source leaves the destination untouched. Both indices must exist or
     * std::runtime_error is thrown. Nonempty copies inherit write() propagation and
     * byte-storage preconditions; no coercion is performed.
     */
    void copy(uint8_t from_argument_index, uint8_t to_argument_index);

    /**
     * @brief Exposes the ordered, non-owning child pointers populated by expand().
     *
     * Erasing pointers never deletes nodes. Keep them alive while used and preserve
     * all connection targets. Vector mutation can invalidate references to slots;
     * it does not change the IR or reset the expansion state.
     */
    std::vector<function_t*>& children();

    /**
     * @brief Exposes owned ports for caller-controlled allocation and configuration.
     *
     * Starts empty. Keep at most 256 ports, retain every connected destination
     * index, and obey argument_t payload invariants. Reallocation invalidates port
     * and name references; erasure/reordering can invalidate recorded port indices.
     */
    std::vector<argument_t>& arguments();

    /**
     * @brief Replaces the borrowed registry, IR and callback, then requests expansion.
     *
     * A null callback throws std::invalid_argument before mutation. On an expanded
     * node, a different function ID calls shrink() and currently throws before
     * replacement; the same ID replaces the description/callback without rebuilding
     * children or connections. Existing port bytes, names and layout are retained:
     * the replacement registry must preserve their type IDs and connected-node use.
     * Expansion failures may leave replacement state and partial children installed.
     */
    void morph(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem, m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t function_ir, void (*call)(function_t&, uint8_t));

    /**
     * @brief Resolves IR child IDs into borrowed nodes and installs directed connections in IR order.
     *
     * Requires registered function_id_t and function_t* types and a coercion between
     * them when there are children. The resolver must return non-null live nodes,
     * with all referenced ports already allocated and lifetimes managed externally.
     * Child rectangles are applied and finalized; parent pointers are not assigned.
     * Connection indices and the enclosing-node sentinel are defined by
     * m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t::connection_info_t.
     *
     * A completed expansion is a no-op on later calls. Connecting stored data can
     * invoke callbacks before expansion completes; avoid reentering this expansion.
     * No rollback is provided on failure: retained children or links may be partial.
     * @throws std::logic_error If unexpanded but children() is already nonempty.
     * @throws std::runtime_error If a child resolves to null or a connection index is invalid.
     *
     * Coercion, child finalize_dimensions(), allocation and connection callback
     * exceptions also propagate; there is no automatic cleanup or retry protocol.
     */
    void expand();
    /**
     * @brief Leaves an unexpanded node unchanged and currently rejects shrinking an expanded node.
     * @throws std::runtime_error On every expanded node, because shrinking is unimplemented.
     *
     * No children or connections are removed when this exception is thrown.
     */
    void shrink();

    /// @brief Returns the runtime rectangle's left edge in enclosing coordinates.
    int left();
    /// @brief Returns the runtime rectangle's right edge in enclosing coordinates.
    int right();
    /// @brief Returns the runtime rectangle's top edge in enclosing coordinates.
    int top();
    /// @brief Returns the runtime rectangle's bottom edge in enclosing coordinates.
    int bottom();

    /// @brief Sets the left edge; call finalize_dimensions() again before coordinate conversion.
    void left(int left);
    /// @brief Sets the right edge; call finalize_dimensions() again before coordinate conversion.
    void right(int right);
    /// @brief Sets the top edge; call finalize_dimensions() again before coordinate conversion.
    void top(int top);
    /// @brief Sets the bottom edge; call finalize_dimensions() again before coordinate conversion.
    void bottom(int bottom);

    /**
     * @brief Caches an aspect-preserving child coordinate extent with its longer side equal to 32767.
     *
     * Uses right-left and bottom-top; both differences must be representable as int
     * and strictly positive. Units follow the enclosing layout, not a fixed pixel
     * scale. Child coordinates are centered on zero, with x rightward and y downward.
     * Edge setters do not invalidate the cache: finalize again after changing edges.
     * @throws std::invalid_argument If either computed dimension is nonpositive.
     */
    void finalize_dimensions();

    /// @brief Returns the cached child-space width; throws std::logic_error before finalization.
    float coordinate_system_width();
    /// @brief Returns the cached child-space height; throws std::logic_error before finalization.
    float coordinate_system_height();

    /**
     * @brief Maps enclosing x to centered child x, truncating the result toward zero.
     *
     * Requires finalized current edges; throws std::logic_error before finalization.
     * No clamping occurs. Keep integer differences and the result representable as int.
     */
    int to_child_x(int x);

    /**
     * @brief Maps enclosing y to centered child y, truncating the result toward zero.
     *
     * Uses the same finalization, range and exception requirements as to_child_x().
     */
    int to_child_y(int y);

    /**
     * @brief Maps centered child x to enclosing x, truncating the result toward zero.
     *
     * Uses the same finalization, range and exception requirements as to_child_x().
     */
    int from_child_x(int x);

    /**
     * @brief Maps centered child y to enclosing y, truncating the result toward zero.
     *
     * Uses the same finalization, range and exception requirements as to_child_x().
     */
    int from_child_y(int y);

private:
    template <typename T>
    T read(uint8_t argument_index);

private:
    m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t* m_typesystem;
    m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t m_function_ir;
    void (*m_call)(function_t&, uint8_t);
    std::vector<function_t*> m_children;
    function_t* m_parent;

    bool m_is_expanded;

    int m_left;
    int m_right;
    int m_top;
    int m_bottom;

    float m_coordinate_system_width;
    float m_coordinate_system_height;

    bool m_is_dimensions_finalized;

    std::vector<argument_t> m_arguments;
};

template <typename T>
T function_t::read(uint8_t argument_index) {
    if (m_arguments.size() <= argument_index) {
        throw std::runtime_error(std::format("argument_index out of range: {}", argument_index));
    }
    argument_t& argument = m_arguments[argument_index];
    if (argument.m_data_type_id == -1) {
        throw std::runtime_error(std::format("argument {} has no data", argument_index));
    }
    if (argument.m_data.size() != m_typesystem->sizeof_type(argument.m_data_type_id)) {
        throw std::runtime_error(std::format("argument {} has an invalid data size", argument_index));
    }
    return m_typesystem->coerce<T>((void*) argument.m_data.data(), argument.m_data_type_id);
}

template <typename T>
void function_t::write(uint8_t argument_index, T data) {
    m_typesystem->register_type<T>();

    write(argument_index, (void*) &data, m_typesystem->type_id<T>());
}

} // namespace m03ge9ij49xkr5obofujoj7ltw_function_runtime

#endif // M03GE9IJ49XKR5OBOFUJOJ7LTW_FUNCTION_RUNTIME_FUNCTION_H
