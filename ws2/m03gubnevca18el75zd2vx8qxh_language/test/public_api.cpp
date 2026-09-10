#include <m03gubnevca18el75zd2vx8qxh_language/language.h>
#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <functional>
#include <stdexcept>
#include <string_view>

namespace lisp = m03gubnevca18el75zd2vx8qxh_language;
namespace runtime = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime;
namespace graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;

static void expect(bool condition) {
    if (!condition) {
        throw std::runtime_error("Lisp public contract check failed");
    }
}

static void expect_failure(const std::function<void()>& operation) {
    try { operation(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Lisp accepted an invalid program or value");
}

int main() {
    const auto context = graph::invocation_context();
    graph::workspace_graph_t workspace_graph(context.workspace_root, context.artifact_root);
    auto evaluate = [&](std::string_view program) { return lisp::evaluate(workspace_graph, program); };
    expect(lisp::print(evaluate(R"((define id (lambda (x) x)) (id "ok"))")) == "ok");
    expect(lisp::print(evaluate(R"((define make (lambda (x) (lambda () x))) (define get-x (make "captured")) (let ((x "outer")) (get-x)))")) == "captured");
    expect(lisp::print(evaluate(R"((let ((x true)) (if x "yes" unknown)))")) == "yes");
    expect(lisp::print(evaluate(R"((equal? (list "x" true) (list "x" true)))")) == "true");
    expect(lisp::print(evaluate(R"((get (record "answer" "yes") "answer"))")) == "yes");
    const auto forms = lisp::read(R"((list "hello" true))");
    expect(lisp::print_syntax(lisp::syntax_from_value(lisp::syntax_value(forms))) == lisp::print_syntax(forms));
    expect_failure([&] { lisp::read("("); });
    expect_failure([&] { lisp::read(")"); });
    expect_failure([&] { evaluate("(if \"not-bool\" true false)"); });
    expect_failure([&] { evaluate("(lambda (x) x)"); });
    expect_failure([&] { evaluate("(get (record) \"missing\")"); });
    expect_failure([&] { evaluate("(record \"x\" true \"x\" false)"); });
    expect_failure([&] { evaluate("id"); });
    return 0;
}
