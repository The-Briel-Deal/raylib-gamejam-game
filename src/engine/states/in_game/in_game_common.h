#pragma once

#include "../game/game.h"
#include "../game_state_controller.h"
#include "../../current_game_info.h"

#include <raylib.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include <glm/glm.hpp>

namespace in_game {
    constexpr i32 UNPAUSE_DELAY_FRAMES = 2;
    constexpr i32 POST_UNPAUSE_HIDDEN_FRAMES = 2;
    constexpr f32 MOUSE_MOTION_MIX = 0.25f;
    constexpr f32 MAX_MOUSE_DELTA_PER_FRAME = 96.0f;
    constexpr f32 MOUSE_DELTA_EPSILON = 0.001f;

    constexpr f32 HEX_SIZE = 1.0f;
    constexpr f32 HEX_SQRT3 = 1.7320508075688772935f;
    constexpr f32 HEX_APOTHEM = HEX_SIZE * 0.86602540378443864676f;

    constexpr f32 FACE_EPS = 0.000001f;
    constexpr f32 TIE_EPS = 0.00001f;
    constexpr f32 STEP_EPS = 0.00001f;

    struct HexCell {
        i32 q;
        i32 r;
    };

    struct HexStepResult {
        f32 t;
        i32 edge;
        i32 next_q;
        i32 next_r;
        bool vertex_tie;

        i32 edge_count;
        i32 edges[6];
    };

    Color VecToColor(glm::vec3 vec);
    f32vec3 U32ToVec(u32 u);
    f32vec3 ColorToVec(Color u);

    glm::vec2 HexAxialToWorld(f32 q, f32 r);
    glm::vec2 HexWorldVectorToAxial(glm::vec2 v);
    HexCell HexRound(f32 q, f32 r);
    bool HexRayStep(i32 q, i32 r, glm::vec2 ray_origin, glm::vec2 ray_dir, f32 min_t, HexStepResult& out_step);
    void ResolveVisibleHexTie(HexStepResult& step, i32 q, i32 r, f32 old_height, Level& level);
    f32vec3 HexFaceNormalFromEdge(i32 edge);

    Rectangle PauseScreenRect(f32 screen_w, f32 screen_h);
    Rectangle PauseContinueButton(Rectangle pause_screen);
    Rectangle PauseExitButton(Rectangle pause_screen);

    bool CursorCaptured();
    void StartPause(Game& game);
    void StartUnpause(Game& game);
    glm::vec2 SmoothMouseMotion(Game& game, Vector2 raw_delta);

    bool ProjectEntityRect(const EntityComponent& entity, const Game& game, Rectangle& out_rect);
}
