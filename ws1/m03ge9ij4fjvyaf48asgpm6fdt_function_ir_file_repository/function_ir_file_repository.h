#ifndef M03GE9IJ4FJVYAF48ASGPM6FDT_FUNCTION_IR_FILE_REPOSITORY_FUNCTION_IR_FILE_REPOSITORY_H
# define M03GE9IJ4FJVYAF48ASGPM6FDT_FUNCTION_IR_FILE_REPOSITORY_FUNCTION_IR_FILE_REPOSITORY_H

# include <filesystem>
# include <string>
# include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
# include <m03ge9ij43jyxy821pda20jhwh_typesystem/typesystem.h>
# include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>

namespace m03ge9ij4fjvyaf48asgpm6fdt_function_ir_file_repository {

class function_ir_file_repository_t {
public:
    function_ir_file_repository_t(const std::filesystem::path& directory_path);

    void save(const m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t& function_ir);

    m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t load(const m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t& function_id) const;
    m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t load_latest(const std::string& ns, const std::string& name) const;

private:
    m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t path_to_function_id(const std::filesystem::path& path) const;
    std::filesystem::path function_id_to_path(const m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t& id) const;

private:
    std::filesystem::path m_directory_path;
};

} // namespace m03ge9ij4fjvyaf48asgpm6fdt_function_ir_file_repository

#endif // M03GE9IJ4FJVYAF48ASGPM6FDT_FUNCTION_IR_FILE_REPOSITORY_FUNCTION_IR_FILE_REPOSITORY_H
