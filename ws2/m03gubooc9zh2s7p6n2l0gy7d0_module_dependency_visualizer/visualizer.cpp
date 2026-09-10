#include "api.h"
#include <m03gubnevc9z8dwzxigmj54y25_lisp_module_command/module_command.h>
#include <m03gn8rf3pe86v64vphnaam6rl_source_dependencies/source_dependencies.h>
#include <memory>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gubnevca18el75zd2vx8qxh_language/language.h>
#include <m03gagbht17w4tser1fescqxye_raylib/raylib.h>
#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <format>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace m03gubooc9zh2s7p6n2l0gy7d0_module_dependency_visualizer {

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args);

struct rec_t {
    float left = 0.0f;
    float right = 0.0f;
    float top = 0.0f;
    float bottom = 0.0f;
};

struct tree_node_t {
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t* module = nullptr;
    std::vector<tree_node_t> children;
    bool recursive_reference = false;
    float measured_width = 0.0f;
    float measured_height = 0.0f;
};

struct module_view_t {
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t* module = nullptr;
    rec_t outer;
    rec_t content;
    std::vector<std::size_t> children;
    std::size_t parent = static_cast<std::size_t>(-1);
    std::size_t depth = 0;
    bool recursive_reference = false;
};

struct view_model_t {
    std::unique_ptr<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t> workspace_graph;
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t* target_module = nullptr;
    std::vector<module_view_t> modules;
    std::size_t root = 0;
    rec_t bounds;
};

struct fitted_text_t {
    std::string wrapped;
    float font_size = 0.0f;
    float spacing = 1.0f;
};

struct grid_metrics_t {
    std::size_t columns = 1;
    std::vector<float> column_widths;
    std::vector<float> row_heights;
};

static constexpr float MODULE_MIN_WIDTH = 420.0f;
static constexpr float MODULE_MIN_HEIGHT = 190.0f;
static constexpr float MODULE_HEADER_HEIGHT = 44.0f;
static constexpr float MODULE_PADDING = 24.0f;
static constexpr float MODULE_GAP = 24.0f;

static float width(rec_t rec) {
    return rec.right - rec.left;
}

static float height(rec_t rec) {
    return rec.bottom - rec.top;
}

static rec_t inset(rec_t rec, float amount) {
    return rec_t {
        .left = rec.left + amount,
        .right = rec.right - amount,
        .top = rec.top + amount,
        .bottom = rec.bottom - amount
    };
}

static bool contains(rec_t rec, Vector2 point) {
    return rec.left <= point.x
        && point.x <= rec.right
        && rec.top <= point.y
        && point.y <= rec.bottom;
}

static Rectangle ray_rectangle(rec_t rec) {
    return Rectangle {
        .x = rec.left,
        .y = rec.top,
        .width = width(rec),
        .height = height(rec)
    };
}

static float to_view_x(float x, rec_t view_rec, rec_t world_rec) {
    return (x - world_rec.left) * width(view_rec) / width(world_rec) + view_rec.left;
}

static float to_view_y(float y, rec_t view_rec, rec_t world_rec) {
    return (y - world_rec.top) * height(view_rec) / height(world_rec) + view_rec.top;
}

static float from_view_x(float x, rec_t view_rec, rec_t world_rec) {
    return (x - view_rec.left) * width(world_rec) / width(view_rec) + world_rec.left;
}

static float from_view_y(float y, rec_t view_rec, rec_t world_rec) {
    return (y - view_rec.top) * height(world_rec) / height(view_rec) + world_rec.top;
}

static rec_t to_view(rec_t rec, rec_t view_rec, rec_t world_rec) {
    return rec_t {
        .left = to_view_x(rec.left, view_rec, world_rec),
        .right = to_view_x(rec.right, view_rec, world_rec),
        .top = to_view_y(rec.top, view_rec, world_rec),
        .bottom = to_view_y(rec.bottom, view_rec, world_rec)
    };
}

static rec_t padded_bounds(rec_t bounds, float padding) {
    return rec_t {
        .left = bounds.left - padding,
        .right = bounds.right + padding,
        .top = bounds.top - padding,
        .bottom = bounds.bottom + padding
    };
}

static rec_t fit_camera(rec_t bounds, rec_t view_rec) {
    bounds = padded_bounds(bounds, 80.0f);

    const float view_aspect = std::max(1.0f, width(view_rec)) / std::max(1.0f, height(view_rec));
    float camera_width = std::max(1.0f, width(bounds));
    float camera_height = std::max(1.0f, height(bounds));

    if (camera_width / camera_height < view_aspect) {
        camera_width = camera_height * view_aspect;
    } else {
        camera_height = camera_width / view_aspect;
    }

    const float middle_x = bounds.left + width(bounds) * 0.5f;
    const float middle_y = bounds.top + height(bounds) * 0.5f;

    return rec_t {
        .left = middle_x - camera_width * 0.5f,
        .right = middle_x + camera_width * 0.5f,
        .top = middle_y - camera_height * 0.5f,
        .bottom = middle_y + camera_height * 0.5f
    };
}

static rec_t local_bounds(const module_view_t& module) {
    return rec_t {
        .left = 0.0f,
        .right = width(module.outer),
        .top = 0.0f,
        .bottom = height(module.outer)
    };
}

static rec_t rect_in_current(
    const view_model_t& model,
    std::size_t current,
    std::size_t module
) {
    const auto& current_module = model.modules.at(current);
    const auto& target_module = model.modules.at(module);

    return rec_t {
        .left = target_module.outer.left - current_module.outer.left,
        .right = target_module.outer.right - current_module.outer.left,
        .top = target_module.outer.top - current_module.outer.top,
        .bottom = target_module.outer.bottom - current_module.outer.top
    };
}

static bool contains_rec(rec_t container, rec_t contained) {
    return container.left <= contained.left
        && contained.right <= container.right
        && container.top <= contained.top
        && contained.bottom <= container.bottom;
}

static rec_t translate(rec_t rec, float dx, float dy) {
    return rec_t {
        .left = rec.left + dx,
        .right = rec.right + dx,
        .top = rec.top + dy,
        .bottom = rec.bottom + dy
    };
}

static bool contains_module(
    const std::vector<const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*>& stack,
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module
) {
    return std::find(stack.begin(), stack.end(), &module) != stack.end();
}

static tree_node_t build_tree(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    std::vector<const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*>& stack
) {
    tree_node_t result {
        .module = &module,
        .children = {},
        .recursive_reference = contains_module(stack, module),
        .measured_width = 0.0f,
        .measured_height = 0.0f
    };

    if (result.recursive_reference) {
        return result;
    }

    stack.push_back(&module);
    for (const auto* dependency : m03gn8rf3pe86v64vphnaam6rl_source_dependencies::scan_sources(module, m03gn8rf3pe86v64vphnaam6rl_source_dependencies::library_source_files(module.source_dir()), nullptr, m03gn8rf3pe86v64vphnaam6rl_source_dependencies::dependency_mode_t::MODULE).dependencies) {
        result.children.push_back(build_tree(*dependency, stack));
    }
    stack.pop_back();

    return result;
}

static std::size_t grid_columns(std::size_t child_count) {
    return std::max<std::size_t>(1, static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<float>(child_count)))));
}

static grid_metrics_t grid_metrics(const std::vector<tree_node_t>& children) {
    grid_metrics_t metrics;
    metrics.columns = grid_columns(children.size());
    const std::size_t rows = (children.size() + metrics.columns - 1) / metrics.columns;
    metrics.column_widths.assign(metrics.columns, 0.0f);
    metrics.row_heights.assign(rows, 0.0f);

    for (std::size_t i = 0; i < children.size(); ++i) {
        const std::size_t column = i % metrics.columns;
        const std::size_t row = i / metrics.columns;
        metrics.column_widths[column] = std::max(metrics.column_widths[column], children[i].measured_width);
        metrics.row_heights[row] = std::max(metrics.row_heights[row], children[i].measured_height);
    }

    return metrics;
}

static float sum_dimensions(const std::vector<float>& values) {
    float result = 0.0f;
    for (const auto value : values) {
        result += value;
    }
    return result;
}

static void measure_tree(tree_node_t& node) {
    for (auto& child : node.children) {
        measure_tree(child);
    }

    if (node.children.empty()) {
        node.measured_width = MODULE_MIN_WIDTH;
        node.measured_height = node.recursive_reference ? 150.0f : MODULE_MIN_HEIGHT;
        return ;
    }

    const auto metrics = grid_metrics(node.children);
    const float children_width = sum_dimensions(metrics.column_widths)
        + static_cast<float>(metrics.column_widths.size() - 1) * MODULE_GAP;
    const float children_height = sum_dimensions(metrics.row_heights)
        + static_cast<float>(metrics.row_heights.size() - 1) * MODULE_GAP;

    node.measured_width = std::max(MODULE_MIN_WIDTH, children_width + MODULE_PADDING * 2.0f);
    node.measured_height = std::max(
        MODULE_MIN_HEIGHT,
        MODULE_HEADER_HEIGHT + MODULE_PADDING * 2.0f + children_height
    );
}

static std::size_t layout_tree(
    const tree_node_t& tree,
    rec_t outer,
    std::size_t parent,
    std::size_t depth,
    std::vector<module_view_t>& modules
) {
    const std::size_t node_index = modules.size();
    modules.push_back(module_view_t {
        .module = tree.module,
        .outer = outer,
        .content = rec_t {
            .left = outer.left + MODULE_PADDING,
            .right = outer.right - MODULE_PADDING,
            .top = outer.top + MODULE_HEADER_HEIGHT + MODULE_PADDING,
            .bottom = outer.bottom - MODULE_PADDING
        },
        .children = {},
        .parent = parent,
        .depth = depth,
        .recursive_reference = tree.recursive_reference
    });

    if (tree.children.empty()) {
        return node_index;
    }

    const auto metrics = grid_metrics(tree.children);
    float y = modules[node_index].content.top;
    for (std::size_t row = 0; row < metrics.row_heights.size(); ++row) {
        float x = modules[node_index].content.left;
        for (std::size_t column = 0; column < metrics.columns; ++column) {
            const std::size_t child_position = row * metrics.columns + column;
            if (tree.children.size() <= child_position) {
                break ;
            }

            const auto& child = tree.children[child_position];
            const rec_t child_outer {
                .left = x + (metrics.column_widths[column] - child.measured_width) * 0.5f,
                .right = x + (metrics.column_widths[column] - child.measured_width) * 0.5f + child.measured_width,
                .top = y + (metrics.row_heights[row] - child.measured_height) * 0.5f,
                .bottom = y + (metrics.row_heights[row] - child.measured_height) * 0.5f + child.measured_height
            };
            const auto child_index = layout_tree(child, child_outer, node_index, depth + 1, modules);
            modules[node_index].children.push_back(child_index);

            x += metrics.column_widths[column] + MODULE_GAP;
        }
        y += metrics.row_heights[row] + MODULE_GAP;
    }

    return node_index;
}

static view_model_t build_view_model(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& target_module) {
    std::vector<const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*> stack;
    auto tree = build_tree(target_module, stack);
    measure_tree(tree);

    view_model_t model;
    model.target_module = &target_module;
    model.bounds = rec_t {
        .left = 0.0f,
        .right = tree.measured_width,
        .top = 0.0f,
        .bottom = tree.measured_height
    };
    model.root = layout_tree(
        tree,
        model.bounds,
        static_cast<std::size_t>(-1),
        0,
        model.modules
    );

    return model;
}

static std::string wrap_text_to_width(Font font, const std::string& text, float font_size, float spacing, float max_width) {
    std::istringstream iss(text);
    std::string word;
    std::string current_line;
    std::string result;

    while (iss >> word) {
        const std::string trial = current_line.empty() ? word : current_line + " " + word;
        const Vector2 size = MeasureTextEx(font, trial.c_str(), font_size, spacing);
        if (size.x > max_width && !current_line.empty()) {
            result += current_line + "\n";
            current_line = word;
        } else {
            current_line = trial;
        }
    }

    if (!current_line.empty()) {
        result += current_line;
    }

    return result;
}

static fitted_text_t fit_text_wrapped(Font font, const std::string& text, rec_t rec, float max_font_size, float spacing) {
    if (width(rec) <= 1.0f || height(rec) <= 1.0f) {
        return fitted_text_t {};
    }

    fitted_text_t best_fit {
        .wrapped = text,
        .font_size = 0.0f,
        .spacing = spacing
    };

    float low = 2.0f;
    float high = std::max(low, std::min(max_font_size, height(rec)));

    for (int i = 0; i < 16; ++i) {
        const float mid = (low + high) * 0.5f;
        const std::string wrapped = wrap_text_to_width(font, text, mid, spacing, width(rec));
        const Vector2 dims = MeasureTextEx(font, wrapped.c_str(), mid, spacing);

        if (dims.x <= width(rec) && dims.y <= height(rec)) {
            best_fit = fitted_text_t {
                .wrapped = wrapped,
                .font_size = mid,
                .spacing = spacing
            };
            low = mid;
        } else {
            high = mid;
        }
    }

    return best_fit;
}

static void draw_centered_text(Font font, const std::string& text, rec_t rec, float max_font_size, Color color) {
    rec = inset(rec, 4.0f);
    if (width(rec) <= 8.0f || height(rec) <= 8.0f) {
        return ;
    }

    const auto text_fit = fit_text_wrapped(font, text, rec, max_font_size, 1.0f);
    if (text_fit.font_size < 3.0f || text_fit.wrapped.empty()) {
        return ;
    }

    const Vector2 size = MeasureTextEx(font, text_fit.wrapped.c_str(), text_fit.font_size, text_fit.spacing);
    const Vector2 pos {
        .x = rec.left + (width(rec) - size.x) * 0.5f,
        .y = rec.top + (height(rec) - size.y) * 0.5f
    };

    DrawTextEx(font, text_fit.wrapped.c_str(), pos, text_fit.font_size, text_fit.spacing, color);
}

static Color fill_color(std::size_t depth, bool recursive_reference) {
    if (recursive_reference) {
        return Color { 255, 239, 232, 255 };
    }

    switch (depth % 5) {
        case 0: return Color { 246, 247, 249, 255 };
        case 1: return Color { 232, 241, 255, 255 };
        case 2: return Color { 230, 246, 239, 255 };
        case 3: return Color { 255, 247, 221, 255 };
        default: return Color { 240, 236, 250, 255 };
    }
}

static Color border_color(std::size_t depth, bool recursive_reference) {
    if (recursive_reference) {
        return Color { 177, 77, 52, 255 };
    }

    switch (depth % 5) {
        case 0: return Color { 42, 47, 53, 255 };
        case 1: return Color { 75, 112, 177, 255 };
        case 2: return Color { 56, 128, 91, 255 };
        case 3: return Color { 151, 111, 33, 255 };
        default: return Color { 102, 82, 157, 255 };
    }
}

static Color header_color(std::size_t depth, bool recursive_reference) {
    if (recursive_reference) {
        return Color { 218, 132, 105, 255 };
    }

    switch (depth % 5) {
        case 0: return Color { 47, 53, 60, 255 };
        case 1: return Color { 78, 119, 189, 255 };
        case 2: return Color { 61, 141, 99, 255 };
        case 3: return Color { 176, 129, 39, 255 };
        default: return Color { 115, 93, 174, 255 };
    }
}

static rec_t clipped_to_view(rec_t rec, rec_t view_rec) {
    return rec_t {
        .left = std::clamp(rec.left, view_rec.left, view_rec.right),
        .right = std::clamp(rec.right, view_rec.left, view_rec.right),
        .top = std::clamp(rec.top, view_rec.top, view_rec.bottom),
        .bottom = std::clamp(rec.bottom, view_rec.top, view_rec.bottom)
    };
}

static float area_fade(rec_t rec, rec_t view_rec) {
    const auto clipped = clipped_to_view(rec, view_rec);
    if (width(clipped) <= 0.0f || height(clipped) <= 0.0f) {
        return 0.0f;
    }

    const double rec_area = static_cast<double>(width(clipped)) * static_cast<double>(height(clipped));
    const double view_area = static_cast<double>(width(view_rec)) * static_cast<double>(height(view_rec));
    if (view_area <= 0.0) {
        return 0.0f;
    }

    return std::clamp(static_cast<float>(1.0 - rec_area / view_area), 0.3f, 1.0f);
}

static void draw_module(
    Font font,
    const view_model_t& model,
    std::size_t current,
    std::size_t module_index,
    rec_t view_rec,
    rec_t camera
) {
    const auto& module = model.modules.at(module_index);
    const rec_t outer = to_view(rect_in_current(model, current, module_index), view_rec, camera);
    if (
        outer.right < view_rec.left ||
        view_rec.right < outer.left ||
        outer.bottom < view_rec.top ||
        view_rec.bottom < outer.top
    ) {
        return ;
    }

    const float fade = area_fade(outer, view_rec);
    if (fade <= 0.0f) {
        return ;
    }

    const bool is_root = module_index == current;
    const float stroke_width = is_root ? 3.0f : 1.5f;
    DrawRectangleRec(ray_rectangle(outer), Fade(fill_color(module.depth, module.recursive_reference), fade));
    DrawRectangleLinesEx(ray_rectangle(outer), stroke_width, Fade(border_color(module.depth, module.recursive_reference), fade));

    const float header_height = std::clamp(height(outer) * 0.18f, 24.0f, 44.0f);
    const rec_t header_rec {
        .left = outer.left,
        .right = outer.right,
        .top = outer.top,
        .bottom = std::min(outer.bottom, outer.top + header_height)
    };
    DrawRectangleRec(ray_rectangle(header_rec), Fade(header_color(module.depth, module.recursive_reference), fade));
    draw_centered_text(font, module.module->name().unique_name(), header_rec, 20.0f, Fade(RAYWHITE, fade));

    if (module.recursive_reference) {
        const rec_t body_rec {
            .left = outer.left + 8.0f,
            .right = outer.right - 8.0f,
            .top = header_rec.bottom + 8.0f,
            .bottom = outer.bottom - 8.0f
        };
        draw_centered_text(font, "recursive reference", body_rec, 15.0f, Fade(Color { 101, 45, 31, 255 }, fade));
    }
}

static void draw_module_tree(
    Font font,
    const view_model_t& model,
    std::size_t current,
    std::size_t module_index,
    rec_t view_rec,
    rec_t camera
) {
    for (const auto child_index : model.modules.at(module_index).children) {
        draw_module_tree(font, model, current, child_index, view_rec, camera);
    }

    draw_module(font, model, current, module_index, view_rec, camera);
}

static void draw_overlay(Font font, const view_model_t& model, std::size_t current, rec_t overlay_rec) {
    DrawRectangleRec(ray_rectangle(overlay_rec), Color { 31, 36, 42, 255 });

    const std::string title = std::format(
        "Builder dependencies: {}",
        model.modules.at(current).module->name().unique_name()
    );
    DrawTextEx(font, title.c_str(), Vector2 { overlay_rec.left + 18.0f, overlay_rec.top + 16.0f }, 20.0f, 1.0f, RAYWHITE);

    if (width(overlay_rec) < 720.0f) {
        return ;
    }

    const std::string counts = std::format("{} contained modules", model.modules.size());
    const Vector2 counts_size = MeasureTextEx(font, counts.c_str(), 18.0f, 1.0f);
    DrawTextEx(
        font,
        counts.c_str(),
        Vector2 { overlay_rec.right - counts_size.x - 18.0f, overlay_rec.top + 18.0f },
        18.0f,
        1.0f,
        Color { 230, 235, 241, 255 }
    );
}

static void switch_camera_context(
    const view_model_t& model,
    std::size_t& current,
    rec_t& camera
) {
    const auto current_bounds = local_bounds(model.modules.at(current));
    if (!contains_rec(current_bounds, camera)) {
        const auto parent = model.modules.at(current).parent;
        if (parent != static_cast<std::size_t>(-1)) {
            const auto current_in_parent = rect_in_current(model, parent, current);
            camera = translate(camera, current_in_parent.left, current_in_parent.top);
            current = parent;
        } else {
            return ;
        }
    }

    for (const auto child_index : model.modules.at(current).children) {
        const auto child_rect = rect_in_current(model, current, child_index);
        if (contains_rec(child_rect, camera)) {
            camera = translate(camera, -child_rect.left, -child_rect.top);
            current = child_index;
            return ;
        }
    }
}

static void update_camera(
    const view_model_t& model,
    std::size_t& current,
    rec_t& camera,
    rec_t view_rec
) {
    if (width(view_rec) <= 1.0f || height(view_rec) <= 1.0f) {
        return ;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        const Vector2 mouse_delta = GetMouseDelta();
        const float drag_x = mouse_delta.x / width(view_rec) * width(camera);
        const float drag_y = mouse_delta.y / height(view_rec) * height(camera);
        camera.left -= drag_x;
        camera.right -= drag_x;
        camera.top -= drag_y;
        camera.bottom -= drag_y;
    }

    const Vector2 mouse = GetMousePosition();
    const float mouse_wheel = GetMouseWheelMove();
    if (mouse_wheel != 0.0f && contains(view_rec, mouse)) {
        const float factor = std::pow(0.9f, mouse_wheel);
        const float focus_x = from_view_x(mouse.x, view_rec, camera);
        const float focus_y = from_view_y(mouse.y, view_rec, camera);
        const rec_t next {
            .left = focus_x - (focus_x - camera.left) * factor,
            .right = focus_x + (camera.right - focus_x) * factor,
            .top = focus_y - (focus_y - camera.top) * factor,
            .bottom = focus_y + (camera.bottom - focus_y) * factor
        };

        const float min_dimension = 40.0f;
        const auto bounds = local_bounds(model.modules.at(current));
        const float max_dimension = std::max(width(bounds), height(bounds)) * 20.0f + 1000.0f;
        if (
            min_dimension <= width(next) &&
            min_dimension <= height(next) &&
            width(next) <= max_dimension &&
            height(next) <= max_dimension
        ) {
            camera = next;
        }
    }

    switch_camera_context(model, current, camera);
}

static void draw(const view_model_t& model, std::size_t current, rec_t overlay_rec, rec_t world_rec, rec_t camera, Font font) {
    BeginDrawing();

    BeginScissorMode(
        static_cast<int>(overlay_rec.left),
        static_cast<int>(overlay_rec.top),
        static_cast<int>(width(overlay_rec)),
        static_cast<int>(height(overlay_rec))
    );
    draw_overlay(font, model, current, overlay_rec);
    EndScissorMode();

    if (width(world_rec) > 1.0f && height(world_rec) > 1.0f) {
        BeginScissorMode(
            static_cast<int>(world_rec.left),
            static_cast<int>(world_rec.top),
            static_cast<int>(width(world_rec)),
            static_cast<int>(height(world_rec))
        );
        ClearBackground(Color { 250, 251, 252, 255 });

        draw_module_tree(font, model, current, current, world_rec, camera);

        EndScissorMode();
    }
    EndDrawing();
}

static int run_viewer(const view_model_t& model) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    const std::string title = std::format("Builder dependencies: {}", model.target_module->name().unique_name());
    InitWindow(1600, 900, title.c_str());
    SetTargetFPS(60);

    const Font font = GetFontDefault();
    std::size_t current = model.root;
    rec_t camera;
    bool camera_initialized = false;

    while (!WindowShouldClose()) {
        const rec_t window_rec {
            .left = 0.0f,
            .right = static_cast<float>(GetScreenWidth()),
            .top = 0.0f,
            .bottom = static_cast<float>(GetScreenHeight())
        };
        const rec_t overlay_rec {
            .left = window_rec.left,
            .right = window_rec.right,
            .top = window_rec.top,
            .bottom = std::min(window_rec.bottom, window_rec.top + 60.0f)
        };
        const rec_t world_rec {
            .left = window_rec.left,
            .right = window_rec.right,
            .top = overlay_rec.bottom,
            .bottom = window_rec.bottom
        };

        if (!camera_initialized || IsKeyPressed(KEY_F)) {
            current = model.root;
            camera = fit_camera(local_bounds(model.modules.at(current)), world_rec);
            camera_initialized = true;
        }

        update_camera(model, current, camera, world_rec);
        draw(model, current, overlay_rec, world_rec, camera, font);
    }

    CloseWindow();
    return 0;
}

static std::string parent_name(const view_model_t& model, const module_view_t& module) {
    if (module.parent == static_cast<std::size_t>(-1)) {
        return "<root>";
    }

    return model.modules.at(module.parent).module->name().unique_name();
}

static std::string layout_text(const view_model_t& model) {
    std::ostringstream output;

    output << std::format("target {}\n", model.target_module->name().unique_name());
    output << std::format("contained_modules {}\n", model.modules.size());

    for (std::size_t i = 0; i < model.modules.size(); ++i) {
        const auto& module = model.modules[i];
        output << std::format(
            "module {} parent {} depth {} children {} rect {} {} {} {}{}\n",
            module.module->name().unique_name(),
            parent_name(model, module),
            module.depth,
            module.children.size(),
            module.outer.left,
            module.outer.top,
            module.outer.right,
            module.outer.bottom,
            module.recursive_reference ? " recursive" : ""
        );
    }

    for (const auto& module : model.modules) {
        for (const auto child_index : module.children) {
            output << std::format(
                "contains {} {}\n",
                module.module->name().unique_name(),
                model.modules.at(child_index).module->name().unique_name()
            );
        }
    }

    return output.str();
}

static bool is_dump_layout_flag(const std::string& value) {
    return value == "--dump-layout";
}

static view_model_t build_model_for_target(const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& target_module_name) {
    const auto invocation_context = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::invocation_context();
    auto workspace_graph = std::make_unique<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t>(invocation_context.workspace_root, invocation_context.artifact_root);
    auto& target_module = m03gubnevc9z8dwzxigmj54y25_lisp_module_command::resolve_module(*workspace_graph, target_module_name);
    auto model = build_view_model(target_module);
    model.workspace_graph = std::move(workspace_graph);
    return model;
}

int run(int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        std::cerr << "usage: " << argv[0] << " <target-module> [--dump-layout]\n";
        return 1;
    }

    const bool dump_only = argc == 3 && is_dump_layout_flag(argv[2]);
    if (argc == 3 && !dump_only) {
        std::cerr << "usage: " << argv[0] << " <target-module> [--dump-layout]\n";
        return 1;
    }

    std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t> args;
    args.push_back(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(argv[1]));
    if (dump_only) {
        args.push_back(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(argv[2]));
    }

    const auto result = apply(args);
    m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::publish_result(result);
    if (dump_only) {
        std::cout << m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(result);
    }

    return 0;
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() != 1 && args.size() != 2) {
        throw std::runtime_error(std::format("module_dependency_visualizer::apply: expected 1 or 2 arguments, got {}", args.size()));
    }
    const auto target_module_name = m03gubnevca18el75zd2vx8qxh_language::as_module_name(args[0]);

    const bool dump_only = args.size() == 2;
    if (dump_only) {
        if (!m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(args[1])) {
            throw std::runtime_error(std::format("module_dependency_visualizer::apply: expected option string, got '{}'", args[1].type_module));
        }
        const auto option = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[1]);
        if (!is_dump_layout_flag(option)) {
            throw std::runtime_error(std::format("module_dependency_visualizer::apply: unsupported option '{}'", option));
        }
    }

    const auto model = build_model_for_target(target_module_name);
    if (dump_only) {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(layout_text(model));
    }

    run_viewer(model);
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::unit_value();
}

} // namespace m03gubooc9zh2s7p6n2l0gy7d0_module_dependency_visualizer

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return m03gubooc9zh2s7p6n2l0gy7d0_module_dependency_visualizer::apply(args);
}
