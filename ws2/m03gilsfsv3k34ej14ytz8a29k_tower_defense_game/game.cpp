#include "game.h"

#include <m03gagbht17w4tser1fescqxye_raylib/raylib.h>

#include <random>
#include <format>
#include <iostream>
#include <unordered_set>

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

game_t::game_t():
    m_camera(
        {{0, 800}, {0, 600}},
        {{0, 400}, {0, 200}}
    )
{
    InitWindow(800, 600, "m03gilsfsv3k34ej14ytz8a29k_tower_defense_game");
    SetTargetFPS(60);

    std::unordered_set<m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>> logical_positions;
    while (logical_positions.size() < 10) {
        m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> position = {rand() % 30 - 15, rand() % 30 - 15};
        logical_positions.insert(position);
    }

    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> tile_size = {32, 32};
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
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> camera_view_delta = {0, 0};
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> camera_world_delta = {0, 0};
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> camera_world_lengths_delta = {0, 0};

    const auto camera_world_rect = m_camera.world_rect();
    const auto camera_world_lengths = camera_world_rect.lengths();
    const auto camera_world_horizontal_speed = std::max(1, static_cast<int>(camera_world_lengths[0] * dt));
    const auto camera_world_vertical_speed = std::max(1, static_cast<int>(camera_world_lengths[1] * dt));

    const auto camera_view_rect = m_camera.view_rect();
    const auto camera_view_size = camera_view_rect.lengths();
    const auto camera_view_horizontal_speed = std::max(1, static_cast<int>(camera_view_size[0] * dt));
    const auto camera_view_vertical_speed = std::max(1, static_cast<int>(camera_view_size[1] * dt));

    if (IsKeyDown(KEY_W)) {
        camera_world_delta[1] -= camera_world_vertical_speed;
    }
    if (IsKeyDown(KEY_S)) {
        camera_world_delta[1] += camera_world_vertical_speed;
    }
    if (IsKeyDown(KEY_A)) {
        camera_world_delta[0] -= camera_world_horizontal_speed;
    }
    if (IsKeyDown(KEY_D)) {
        camera_world_delta[0] += camera_world_horizontal_speed;
    }

    if (IsKeyDown(KEY_UP)) {
        camera_view_delta[1] -= camera_view_vertical_speed;
    }
    if (IsKeyDown(KEY_DOWN)) {
        camera_view_delta[1] += camera_view_vertical_speed;
    }
    if (IsKeyDown(KEY_LEFT)) {
        camera_view_delta[0] -= camera_view_horizontal_speed;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        camera_view_delta[0] += camera_view_horizontal_speed;
    }

    if (IsKeyDown(KEY_Q)) {
        camera_world_lengths_delta += m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>{std::max(1, static_cast<int>(camera_world_lengths[0] * dt)), std::max(1, static_cast<int>(camera_world_lengths[1] * dt))};
    }
    if (IsKeyDown(KEY_E)) {
        camera_world_lengths_delta -= m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>{std::max(1, static_cast<int>(camera_world_lengths[0] * dt)), std::max(1, static_cast<int>(camera_world_lengths[1] * dt))};
    }

    std::cout << std::format("camera_world_lengths: {}, camera_world_lengths_delta: {}, Camera world rect: {}, Camera view rect: {}\n", camera_world_lengths, camera_world_lengths_delta, camera_world_rect, camera_view_rect);

    auto prev_camera_world_rect = camera_world_rect;
    auto prev_camera_view_rect = camera_view_rect;
    prev_camera_world_rect += camera_world_delta;
    prev_camera_world_rect = prev_camera_world_rect.inflate(camera_world_lengths_delta);
    prev_camera_view_rect += camera_view_delta;
    m_camera.world_rect() = prev_camera_world_rect;
    m_camera.view_rect() = prev_camera_view_rect;
}

void game_t::render() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    const auto camera_view_rect = m_camera.view_rect();
    const auto camera_view_top_left = camera_view_rect.corner();
    const auto camera_view_size = camera_view_rect.lengths();
    DrawRectangleLines(camera_view_top_left[0], camera_view_top_left[1], camera_view_size[0], camera_view_size[1], RED);

    m_map.for_each_tile([this](m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2> tile_world_rect, tile_t type) {
        m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2> tile_view(m_camera.world_to_view(tile_world_rect));

        const auto rendered_tile_view = tile_view.intersect(m_camera.view_rect());
        if (rendered_tile_view.is_empty()) {
            return;
        }

        const auto rendered_tile_view_top_left = rendered_tile_view.corner();
        const auto rendered_tile_view_size = rendered_tile_view.lengths();

        switch (type) {
            case tile_t::empty: {
                DrawRectangle(rendered_tile_view_top_left[0], rendered_tile_view_top_left[1], rendered_tile_view_size[0], rendered_tile_view_size[1], LIGHTGRAY);
            } break ;
            case tile_t::grass: {
                DrawRectangle(rendered_tile_view_top_left[0], rendered_tile_view_top_left[1], rendered_tile_view_size[0], rendered_tile_view_size[1], GREEN);
            } break ;
            case tile_t::path: {
                DrawRectangle(rendered_tile_view_top_left[0], rendered_tile_view_top_left[1], rendered_tile_view_size[0], rendered_tile_view_size[1], BROWN);
            } break ;
            case tile_t::water: {
                DrawRectangle(rendered_tile_view_top_left[0], rendered_tile_view_top_left[1], rendered_tile_view_size[0], rendered_tile_view_size[1], BLUE);
            } break ;
            case tile_t::mountain: {
                DrawRectangle(rendered_tile_view_top_left[0], rendered_tile_view_top_left[1], rendered_tile_view_size[0], rendered_tile_view_size[1], DARKGRAY);
            } break ;
            default: {
                throw std::runtime_error(std::format("Unknown tile type: {}", static_cast<int>(type)));
            } break ;
        }
    });

    EndDrawing();
}

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
