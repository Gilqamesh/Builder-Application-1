#ifndef M03GE9IJ4FJVYAF48ASGPM6FDT_FUNCTION_IR_FILE_REPOSITORY_FUNCTION_IR_FILE_REPOSITORY_H
# define M03GE9IJ4FJVYAF48ASGPM6FDT_FUNCTION_IR_FILE_REPOSITORY_FUNCTION_IR_FILE_REPOSITORY_H

# include <filesystem>
# include <string>
# include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
# include <m03ge9ij43jyxy821pda20jhwh_typesystem/typesystem.h>
# include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>

namespace m03ge9ij4fjvyaf48asgpm6fdt_function_ir_file_repository {

/**
 * @brief Saves encoded function descriptions in a directory and retrieves them by ID or version time.
 *
 * Each file is `directory_path / function_id_t::to_string(id)`, with no extension
 * or escaping, for example `math::add@42`. Use IDs whose text round-trips through
 * function_id_t::from_string() and forms one legal filename component on the host
 * filesystem. Path separators and absolute paths are not sanitized or rejected.
 * Whole-second filenames collide for IDs differing only in fractional seconds.
 * Keep this directory dedicated to canonical ID filenames for load_latest().
 *
 * The repository owns a copy of the path, borrows no typesystem, and holds no
 * files open between calls. Destruction leaves saved files in place. A relative
 * path is resolved against the working directory at each filesystem operation.
 * Callers must coordinate overlapping reads, saves and external directory changes;
 * no locking or atomic file replacement is provided.
 *
 * Contents use m03ge9ij47sume9p7pg7nbvu88_function_ir_binary::function_ir_binary_t;
 * that header owns byte layout, representation limits and decoding failures.
 * Its current timestamp truncation and omission of the enclosing rectangle also
 * affect repository round-trips.
 *
 * @code{.cpp}
 * #include <m03ge9ij4fjvyaf48asgpm6fdt_function_ir_file_repository/function_ir_file_repository.h>
 * #include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
 * #include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>
 *
 * #include <cassert>
 * #include <filesystem>
 *
 * int main() {
 *     using m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t;
 *     const std::filesystem::path directory_path("function-ir-example");
 *     m03ge9ij4fjvyaf48asgpm6fdt_function_ir_file_repository::function_ir_file_repository_t
 *         repository(directory_path);
 *     m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t function_ir {};
 *     function_ir.function_id = function_id_t::from_string("math::add@42");
 *     repository.save(function_ir);
 *     const auto earlier_id = function_ir.function_id;
 *     function_ir.function_id = function_id_t::from_string("math::add@43");
 *     repository.save(function_ir);
 *     const auto earlier_ir = repository.load(earlier_id);
 *     const auto latest_ir = repository.load_latest("math", "add");
 *     assert(earlier_ir.function_id == earlier_id);
 *     assert(latest_ir.function_id == function_ir.function_id);
 *     // The two files remain in directory_path after repository destruction.
 * }
 * @endcode
 */
class function_ir_file_repository_t {
public:
    /**
     * @brief Copies the directory path and creates missing directories, including parents.
     * Existing paths are not checked to be directories; such failures can surface
     * on save/load/scan. Filesystem exceptions from existence and creation propagate.
     */
    function_ir_file_repository_t(const std::filesystem::path& directory_path);

    /**
     * @brief Encodes an IR and writes its ID-derived file, truncating any existing file at that path.
     * Encoding completes before opening the destination, so representation errors
     * leave an existing file untouched. The input is borrowed only for this call.
     * @throws std::invalid_argument If binary encoding rejects the IR.
     * @throws std::runtime_error If the destination cannot be opened.
     * @warning Current stream handling does not check write/close failure after a
     * successful open; normal return is not a durability guarantee and a failure
     * can leave an existing file truncated or partially replaced.
     */
    void save(const m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t& function_ir);

    /**
     * @brief Reads the ID-derived file and returns an independently owned decoded IR.
     * The current implementation does not compare the encoded ID with the requested
     * ID. The binary decoder's representation losses and structural checks apply.
     * @throws std::runtime_error If opening fails (including a missing file), or
     * decoding finds malformed/truncated records. Filesystem/allocation exceptions
     * can also propagate; stream read/seek failures are not checked separately.
     */
    m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t load(const m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t& function_id) const;
    /**
     * @brief Loads the matching version with the greatest creation time encoded in its filename.
     * Scans immediate regular files, skipping directories and other non-regular
     * entries. Every regular filename is parsed before filtering by ns and name,
     * so even an unrelated malformed filename aborts the scan. Ordering uses
     * parsed whole seconds, not filesystem modification time or file contents.
     * The selected ID is passed to load(), which reconstructs its canonical path.
     * A usable selection must pass function_id_t::operator bool(); an epoch-only
     * version or empty namespace/name therefore cannot be returned as the latest.
     * @throws std::runtime_error If no usable match exists or load() fails.
     * Exceptions from filename parsing and filesystem iteration propagate unchanged.
     */
    m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t load_latest(const std::string& ns, const std::string& name) const;

private:
    m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t path_to_function_id(const std::filesystem::path& path) const;
    std::filesystem::path function_id_to_path(const m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t& id) const;

private:
    std::filesystem::path m_directory_path;
};

} // namespace m03ge9ij4fjvyaf48asgpm6fdt_function_ir_file_repository

#endif // M03GE9IJ4FJVYAF48ASGPM6FDT_FUNCTION_IR_FILE_REPOSITORY_FUNCTION_IR_FILE_REPOSITORY_H
