#include <m03gf09la5rvbh6kk4vvt1qawv_module_shell/module_shell.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>
#include <cstring>

namespace m03gf09la5rvbh6kk4vvt1qawv_module_shell {

void run() {
    const auto module_names = []() {
        const auto context = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context();
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph(context.workspace_root, context.artifact_root);
        return workspace_graph.module_names();
    }();
    
    while (true) {
        char* input = readline("shell> ");
        if (input == nullptr) {
            break ; // exit on EOF
        }

        if (strcmp(input, "ls") == 0) {
            for (const auto& module_name : module_names) {
                printf("%s\n", module_name.c_str());
            }
        } else if (*input) {
            add_history(input);
        }

        // implement tab completion

        free(input);
    }
}

} // namespace m03gf09la5rvbh6kk4vvt1qawv_module_shell
