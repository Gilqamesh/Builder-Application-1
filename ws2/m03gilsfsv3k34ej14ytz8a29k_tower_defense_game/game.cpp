#include "game.h"

#include <m03gagbht17w4tser1fescqxye_raylib/raylib.h>

#include <random>
#include <format>
#include <iostream>
#include <unordered_set>

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

game_t::game_t():
    m_camera(
        {{0, 0}, {800, 600}},
        {{0, 0}, {400, 200}}
    )
{
    InitWindow(800, 600, "m03gilsfsv3k34ej14ytz8a29k_tower_defense_game");
    SetTargetFPS(60);

    std::unordered_set<vec2_t> logical_positions;
    while (logical_positions.size() < 10) {
        vec2_t position = {rand() % 30 - 15, rand() % 30 - 15};
        logical_positions.insert(position);
    }

    const vec2_t tile_size = {32, 32};
    m_map.set_tile_size(tile_size);
    for (const auto& position : logical_positions) {
        const tile_t type = static_cast<tile_t>(rand() % static_cast<int>(tile_t::__SIZE));
        m_map.set_tile(position, type);
    }
}

game_t::~game_t() {
    CloseWindow();
}

void game_t::run() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        update(dt);
        render();
    }
}

void game_t::update(float dt) {
    vec2_t camera_view_delta = {0, 0};
    vec2_t camera_world_delta = {0, 0};
    vec2_t camera_world_size_delta = {0, 0};

    const auto camera_world_rect = m_camera.world_rect();
    const auto camera_world_size = camera_world_rect.size();
    const auto camera_world_horizontal_speed = std::max(1, static_cast<int>(camera_world_size.x * dt));
    const auto camera_world_vertical_speed = std::max(1, static_cast<int>(camera_world_size.y * dt));

    const auto camera_view_rect = m_camera.view_rect();
    const auto camera_view_size = camera_view_rect.size();
    const auto camera_view_horizontal_speed = std::max(1, static_cast<int>(camera_view_size.x * dt));
    const auto camera_view_vertical_speed = std::max(1, static_cast<int>(camera_view_size.y * dt));

    if (IsKeyDown(KEY_W)) {
        camera_world_delta.y -= camera_world_vertical_speed;
    }
    if (IsKeyDown(KEY_S)) {
        camera_world_delta.y += camera_world_vertical_speed;
    }
    if (IsKeyDown(KEY_A)) {
        camera_world_delta.x -= camera_world_horizontal_speed;
    }
    if (IsKeyDown(KEY_D)) {
        camera_world_delta.x += camera_world_horizontal_speed;
    }

    if (IsKeyDown(KEY_UP)) {
        camera_view_delta.y -= camera_view_vertical_speed;
    }
    if (IsKeyDown(KEY_DOWN)) {
        camera_view_delta.y += camera_view_vertical_speed;
    }
    if (IsKeyDown(KEY_LEFT)) {
        camera_view_delta.x -= camera_view_horizontal_speed;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        camera_view_delta.x += camera_view_horizontal_speed;
    }

    if (IsKeyDown(KEY_Q)) {
        camera_world_size_delta += camera_world_size * dt;
    }
    if (IsKeyDown(KEY_E)) {
        camera_world_size_delta -= camera_world_size * dt;
    }

    auto prev_camera_world_rect = camera_world_rect;
    auto prev_camera_view_rect = camera_view_rect;
    prev_camera_world_rect += camera_world_delta;
    prev_camera_world_rect.inflate(camera_world_size_delta);
    prev_camera_view_rect += camera_view_delta;
    m_camera.world_rect(prev_camera_world_rect);
    m_camera.view_rect(prev_camera_view_rect);
}

void game_t::render() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    const auto camera_view_rect = m_camera.view_rect();
    const auto camera_view_top_left = camera_view_rect.top_left();
    const auto camera_view_size = camera_view_rect.size();
    DrawRectangleLines(camera_view_top_left.x, camera_view_top_left.y, camera_view_size.x, camera_view_size.y, RED);

    m_map.for_each_tile([this](rect_t tile_world_rect, tile_t type) {
        rect_t tile_view(
            m_camera.world_to_view(tile_world_rect.top_left()),
            m_camera.world_to_view(tile_world_rect.bottom_right())
        );

        rect_t rendered_tile_view = tile_view.intersect(m_camera.view_rect());
        if (rendered_tile_view.is_empty()) {
            return;
        }

        switch (type) {
            case tile_t::empty: {
                DrawRectangle(rendered_tile_view.top_left().x, rendered_tile_view.top_left().y, rendered_tile_view.size().x, rendered_tile_view.size().y, LIGHTGRAY);
            } break ;
            case tile_t::grass: {
                DrawRectangle(rendered_tile_view.top_left().x, rendered_tile_view.top_left().y, rendered_tile_view.size().x, rendered_tile_view.size().y, GREEN);
            } break ;
            case tile_t::path: {
                DrawRectangle(rendered_tile_view.top_left().x, rendered_tile_view.top_left().y, rendered_tile_view.size().x, rendered_tile_view.size().y, BROWN);
            } break ;
            case tile_t::water: {
                DrawRectangle(rendered_tile_view.top_left().x, rendered_tile_view.top_left().y, rendered_tile_view.size().x, rendered_tile_view.size().y, BLUE);
            } break ;
            case tile_t::mountain: {
                DrawRectangle(rendered_tile_view.top_left().x, rendered_tile_view.top_left().y, rendered_tile_view.size().x, rendered_tile_view.size().y, DARKGRAY);
            } break ;
            default: {
                throw std::runtime_error(std::format("Unknown tile type: {}", static_cast<int>(type)));
            } break ;
        }
    });

    EndDrawing();
}

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
