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

static std::multimap<std::string, m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t> module_names_by_friendly_name;
static std::unordered_map<std::string, m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t> module_name_by_module_unique_name;

char* character_name_generator(const char* text, int state) {
    static auto it = module_names_by_friendly_name.end();
    static size_t len;

    if (state == 0) {
        it = module_names_by_friendly_name.begin();
        len = strlen(text);
    }

    while (it != module_names_by_friendly_name.end()) {
        const auto& [friendly_name, _] = *it;
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

static void skip_whitespace(std::string& string, size_t& pos) {
    while (pos < string.size() && isspace(string[pos])) {
        ++pos;
    }
}

static std::vector<std::string> parse_tokens(std::string input) {
    std::vector<std::string> tokens;

    size_t pos = 0;
    while (pos < input.size()) {
        skip_whitespace(input, pos);
        if (input.size() <= pos) {
            break ;
        }

        size_t start = pos;
        size_t end = pos;
        switch (input[pos]) {
            case '"':
            case '\'': {
                char quote_char = input[pos];
                ++pos; // Skip the opening quote
                if (input.size() <= pos) {
                    throw std::runtime_error(std::format("Unmatched quote {} in input", quote_char));
                }

                start = pos;
                while (pos < input.size() && input[pos] != quote_char) {
                    ++pos;
                }

                if (input.size() <= pos || input[pos] != quote_char) {
                    throw std::runtime_error(std::format("Unmatched quote {} in input", quote_char));
                }

                end = pos;
                ++pos; // Skip the closing quote
            } break;
            default: {
                while (pos < input.size() && !isspace(input[pos])) {
                    ++pos;
                }
                end = pos;
            } break ;
        }

        if (start == end) {
            throw std::runtime_error(std::format("Unexpected whitespace in input"));
        }

        auto token = input.substr(start, end - start);
        auto it = module_names_by_friendly_name.find(token);
        if (it != module_names_by_friendly_name.end()) {
            const auto& [_, module_name] = *it;
            token = module_name.unique_name();
        }
        tokens.emplace_back(token);
    }

    return tokens;
}

void run() {
    {
        const auto context = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context();
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph(context.workspace_root, context.artifact_root);
        const auto module_names = workspace_graph.module_names();
        for (const auto& module_name : module_names) {
            module_names_by_friendly_name.emplace(module_name.friendly_name(), module_name);
            module_name_by_module_unique_name.emplace(module_name.unique_name(), module_name);
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
            for (const auto& [friendly_name, _] : module_names_by_friendly_name) {
                std::cout << std::format("{}\n", friendly_name);
            }
        } else if (*input) {
            add_history(input);
        }

        const auto tokens = parse_tokens(input);

        if (tokens.empty()) {
            free(input);
            continue;
        }

        try {
            auto it = module_name_by_module_unique_name.find(tokens[0]);
            if (it != module_name_by_module_unique_name.end()) {
                const auto& [_, module_name] = *it;
                m03gagbhst621faiop1rztfkqp_builder_cli::create_and_wait_checked(
                    module_name,
                    std::vector<std::string>(tokens.begin() + 1, tokens.end())
                );
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }

        free(input);
    }
}

} // namespace m03gf09la5rvbh6kk4vvt1qawv_module_shell
