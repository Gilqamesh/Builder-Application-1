#ifndef M03GE9SCIYP8Y22MZR4NME82TM_MODULE_DEPENDENCY_IR_MODULE_H
# define M03GE9SCIYP8Y22MZR4NME82TM_MODULE_DEPENDENCY_IR_MODULE_H

# include <vector>
# include <string>

namespace m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir {

struct module_t {
    std::string name;
    std::vector<std::string> module_dependencies;
    std::vector<std::string> builder_dependencies;
};

struct workspace_t {
    std::string name;
    std::vector<module_t> modules;
};

struct module_dependency_ir_t {
    std::vector<workspace_t> workspaces;
};

} // namespace m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir

#endif // M03GE9SCIYP8Y22MZR4NME82TM_MODULE_DEPENDENCY_IR_MODULE_H
