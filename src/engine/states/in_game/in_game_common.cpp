#include "in_game_common.h"

using namespace glm;

namespace in_game {
Color VecToColor(vec3 vec) {
    Color c;
    c.a = 255;

    vec = clamp(vec, 0.0f, 1.0f);
    vec = vec * 255.0f;

    c.r = (char)vec.x;
    c.g = (char)vec.y;
    c.b = (char)vec.z;
    return c;
}

f32vec3 U32ToVec(u32 u) {
    auto vec = f32vec3{};
    vec.r = (u & 255) / 255.0f;
    vec.g = ((u >> 8) & 255) / 255.0f;
    vec.b = ((u >> 16) & 255) / 255.0f;
    return vec;
}

f32vec3 ColorToVec(Color u) {
    auto vec = f32vec3{};
    vec.r = (float)(u.r) / 255.0f;
    vec.g = (float)(u.g) / 255.0f;
    vec.b = (float)(u.b) / 255.0f;
    return vec;
}

    const i32 HEX_NEIGHBOR_Q[6] = {
         0, -1, -1,  0,  1,  1
    };

    const i32 HEX_NEIGHBOR_R[6] = {
         1,  1,  0, -1, -1,  0
    };

    const vec2 HEX_FACE_NORMAL_2D[6] = {
        vec2( 0.5f,  0.86602540378443864676f),
        vec2(-0.5f,  0.86602540378443864676f),
        vec2(-1.0f,  0.0f),
        vec2(-0.5f, -0.86602540378443864676f),
        vec2( 0.5f, -0.86602540378443864676f),
        vec2( 1.0f,  0.0f),
    };

    const f32vec3 HEX_FACE_NORMAL_3D[6] = {
        f32vec3(-0.5f, 0.0f, -0.86602540378443864676f),
        f32vec3( 0.5f, 0.0f, -0.86602540378443864676f),
        f32vec3( 1.0f, 0.0f, -0.0f),
        f32vec3( 0.5f, 0.0f,  0.86602540378443864676f),
        f32vec3(-0.5f, 0.0f,  0.86602540378443864676f),
        f32vec3(-1.0f, 0.0f, -0.0f),
    };

    vec2 HexAxialToWorld(f32 q, f32 r) {
        return vec2(
            HEX_SIZE * HEX_SQRT3 * (q + r * 0.5f),
            HEX_SIZE * 1.5f * r
        );
    }

    vec2 HexWorldVectorToAxial(vec2 v) {
        f32 r = v.y / (HEX_SIZE * 1.5f);
        f32 q = v.x / (HEX_SIZE * HEX_SQRT3) - r * 0.5f;
        return vec2(q, r);
    }

    HexCell HexRound(f32 q, f32 r) {
        f32 cube_x = q;
        f32 cube_z = r;
        f32 cube_y = -cube_x - cube_z;

        i32 rx = (i32)glm::round(cube_x);
        i32 ry = (i32)glm::round(cube_y);
        i32 rz = (i32)glm::round(cube_z);

        f32 x_diff = glm::abs((f32)rx - cube_x);
        f32 y_diff = glm::abs((f32)ry - cube_y);
        f32 z_diff = glm::abs((f32)rz - cube_z);

        if (x_diff > y_diff && x_diff > z_diff) {
            rx = -ry - rz;
        } else if (y_diff > z_diff) {
            ry = -rx - rz;
        } else {
            rz = -rx - ry;
        }

        return HexCell{ rx, rz };
    }

    bool HexRayStep(
        i32 q,
        i32 r,
        vec2 ray_origin,
        vec2 ray_dir,
        f32 min_t,
        HexStepResult& out_step
    ) {
        vec2 center = HexAxialToWorld((f32)q, (f32)r);
        vec2 origin_from_center = ray_origin - center;

        f32 best_t = INFINITY;
        i32 best_edges[6] = {};
        i32 best_count = 0;

        for (i32 edge = 0; edge < 6; edge++) {
            const vec2 n = HEX_FACE_NORMAL_2D[edge];
            f32 denom = dot(ray_dir, n);

            if (denom <= FACE_EPS) {
                continue;
            }

            f32 numer = HEX_APOTHEM - dot(origin_from_center, n);
            f32 t = numer / denom;

            if (t <= min_t) {
                continue;
            }

            if (t < best_t - TIE_EPS) {
                best_t = t;
                best_edges[0] = edge;
                best_count = 1;
            } else if (glm::abs(t - best_t) <= TIE_EPS && best_count < 6) {
                best_edges[best_count++] = edge;
            }
        }

        if (best_count <= 0 || glm::isnan(best_t) || glm::isinf(best_t)) {
            return false;
        }

        out_step.t = best_t;
        out_step.edge_count = best_count;
        out_step.vertex_tie = best_count > 1;

        for (i32 i = 0; i < best_count; i++) {
            out_step.edges[i] = best_edges[i];
        }

        i32 best_edge = best_edges[0];
        f32 best_score = -INFINITY;

        for (i32 i = 0; i < best_count; i++) {
            i32 edge = best_edges[i];
            f32 score = dot(ray_dir, HEX_FACE_NORMAL_2D[edge]);
            if (score > best_score) {
                best_score = score;
                best_edge = edge;
            }
        }

        out_step.edge = best_edge;
        out_step.next_q = q + HEX_NEIGHBOR_Q[best_edge];
        out_step.next_r = r + HEX_NEIGHBOR_R[best_edge];

        return true;
    }

    void ResolveVisibleHexTie(
        HexStepResult& step,
        i32 q,
        i32 r,
        f32 old_height,
        Level& level
    ) {
        if (!step.vertex_tie || step.edge_count <= 1) {
            return;
        }

        i32 chosen_edge = step.edge;
        f32 best_rise = -INFINITY;

        for (i32 i = 0; i < step.edge_count; i++) {
            i32 edge = step.edges[i];

            i32 nq = q + HEX_NEIGHBOR_Q[edge];
            i32 nr = r + HEX_NEIGHBOR_R[edge];

            f32 h = 0.0f;
            if (level.InBounds(nq, nr)) {
                h = level.At(nq, nr);
            }

            f32 rise = h - old_height;

            if (rise > best_rise) {
                best_rise = rise;
                chosen_edge = edge;
            }
        }

        step.edge = chosen_edge;
        step.next_q = q + HEX_NEIGHBOR_Q[chosen_edge];
        step.next_r = r + HEX_NEIGHBOR_R[chosen_edge];
    }

    f32vec3 HexFaceNormalFromEdge(i32 edge) {
        return HEX_FACE_NORMAL_3D[edge];
    }

    Rectangle PauseScreenRect(f32 screen_w, f32 screen_h) {
        return Rectangle{
            screen_w * 0.25f,
            screen_h * 0.25f,
            screen_w * 0.5f,
            screen_h * 0.5f
        };
    }

    bool CursorCaptured() {
#if defined(PLATFORM_WEB)
        return EM_ASM_INT({
            return document.pointerLockElement ? 1 : 0;
        }) != 0;
#else
        return IsCursorHidden();
#endif
    }

    void StartPause(Game& game) {
        game.paused = true;
        game.unpause_delay_frames = 0;
        game.pause_blocked_after_unpause = false;
        game.cursor_hidden_after_unpause_frames = 0;
        game.mouse_motion = vec2(0.0f);

        if (!game.pause_cursor_released) {
            EnableCursor();
            game.pause_cursor_released = true;
        }
    }

    void StartUnpause(Game& game) {
        game.unpause_delay_frames = UNPAUSE_DELAY_FRAMES;
        game.pause_cursor_released = false;
        game.pause_blocked_after_unpause = true;
        game.cursor_hidden_after_unpause_frames = 0;
        game.mouse_motion = vec2(0.0f);
        DisableCursor();
    }

    vec2 SmoothMouseMotion(Game& game, Vector2 raw_delta) {
        vec2 incoming_motion(
            glm::clamp(raw_delta.x, -MAX_MOUSE_DELTA_PER_FRAME, MAX_MOUSE_DELTA_PER_FRAME),
            glm::clamp(raw_delta.y, -MAX_MOUSE_DELTA_PER_FRAME, MAX_MOUSE_DELTA_PER_FRAME)
        );

        game.mouse_motion = mix(game.mouse_motion, incoming_motion, MOUSE_MOTION_MIX);

        if (glm::abs(game.mouse_motion.x) < MOUSE_DELTA_EPSILON) {
            game.mouse_motion.x = 0.0f;
        }
        if (glm::abs(game.mouse_motion.y) < MOUSE_DELTA_EPSILON) {
            game.mouse_motion.y = 0.0f;
        }

        return game.mouse_motion;
    }
}
