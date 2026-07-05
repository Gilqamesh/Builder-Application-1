#include <m03gf09la5rvbh6kk4vvt1qawv_module_shell/module_shell.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

#include <unordered_map>

#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>
#include <cstring>

namespace m03gf09la5rvbh6kk4vvt1qawv_module_shell {

static std::unordered_map<std::string, std::string> human_slug_by_module_name;

char* character_name_generator(const char* text, int state) {
    static std::unordered_map<std::string, std::string>::iterator it = human_slug_by_module_name.end();
    static size_t len;

    if (state == 0) {
        it = human_slug_by_module_name.begin();
        len = strlen(text);
    }

    while (it != human_slug_by_module_name.end()) {
        const std::string& name = it->first;
        ++it;
        if (name.compare(0, len, text) == 0) {
            return strdup(name.c_str());
        }
    }

    return nullptr;
}

char** attempted_completion_function(const char* text, int start, int end) {
    rl_attempted_completion_over = 1; // Prevent default filename completion
    return rl_completion_matches(text, character_name_generator);
}

void run() {
    module_names = []() {
        const auto context = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context();
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph(context.workspace_root, context.artifact_root);
        const auto module_names = workspace_graph.module_names();
        std::vector<std::string> result;
        for (const auto& module_name : module_names) {
            result.push_back(module_name);
        }
        return result;
    }();

    rl_attempted_completion_function = attempted_completion_function;
    rl_completer_quote_characters = "\"'";
    
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
