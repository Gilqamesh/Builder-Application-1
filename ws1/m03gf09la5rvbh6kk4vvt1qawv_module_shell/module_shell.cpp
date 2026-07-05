#include <m03gf09la5rvbh6kk4vvt1qawv_module_shell/module_shell.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbhst621faiop1rztfkqp_builder_cli/builder_cli.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <readline/readline.h>
#include <readline/history.h>

#include <iostream>
#include <map>

namespace m03gf09la5rvbh6kk4vvt1qawv_module_shell {

static std::map<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t, std::string> friendly_name_by_module_name;

char* character_name_generator(const char* text, int state) {
    static auto it = friendly_name_by_module_name.end();
    static size_t len;

    if (state == 0) {
        it = friendly_name_by_module_name.begin();
        len = strlen(text);
    }

    while (it != friendly_name_by_module_name.end()) {
        const auto& [_, friendly_name] = *it;
        ++it;
        if (friendly_name.compare(0, len, text) == 0) {
            return strdup(friendly_name.c_str());
        }
    }

    return nullptr;
}

char** attempted_completion_function(const char* text, int start, int end) {
    rl_attempted_completion_over = 1; // Prevent default filename completion
    return rl_completion_matches(text, character_name_generator);
}

void run() {
    {
        const auto context = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context();
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph(context.workspace_root, context.artifact_root);
        const auto module_names = workspace_graph.module_names();
        for (const auto& module_name : module_names) {
            friendly_name_by_module_name.emplace(module_name, module_name.friendly_name());
        }
    }

    rl_attempted_completion_function = attempted_completion_function;
    rl_completer_quote_characters = "\"'";
    
    while (true) {
        char* input = readline("shell> ");
        if (input == nullptr) {
            break ; // exit on EOF
        }

        if (strcmp(input, "ls") == 0) {
            for (const auto& [_, friendly_name] : friendly_name_by_module_name) {
                std::cout << std::format("{}\n", friendly_name);
            }
        } else if (*input) {
            add_history(input);
        }

        auto it = friendly_name_by_module_name.find(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t(input));
        if (it != friendly_name_by_module_name.end()) {
            const auto& [module_name, _] = *it;
            // void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, const std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t>& args);
            m03gagbhst621faiop1rztfkqp_builder_cli::create_and_wait_checked(module_name, {});
        }

        free(input);
    }
}

} // namespace m03gf09la5rvbh6kk4vvt1qawv_module_shell
