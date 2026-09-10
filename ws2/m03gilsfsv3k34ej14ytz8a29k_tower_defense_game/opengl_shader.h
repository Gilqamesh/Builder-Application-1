#ifndef M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_OPENGL_SHADER_H
# define M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_OPENGL_SHADER_H

# include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>

# include <m03gl22hn0dqmosreqjie9tg5m_opengl_renderer/api.h>

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

/**
 * @brief Experimental declaration for translating a shader AST to OpenGL.
 *
 * Its constructor has no definition in this module. This standalone wrapper is legacy/experimental, as specified in AGENTS.md.
 * See api.h and game_t for the active software-renderer path.
 */
class opengl_shader_t {
public:
    explicit opengl_shader_t(const m03gsy25j4v7nccgmsdov9ioft_shader::shader_ast_t& shader_ast);

private:
    m03gl22hn0dqmosreqjie9tg5m_opengl_renderer::opengl_renderer_t* m_opengl_renderer;
};

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

#endif // M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_OPENGL_SHADER_H
