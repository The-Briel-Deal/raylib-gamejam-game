#include "game/game.h"
#include "game_state_controller.h"
#include "../current_game_info.h"
#include <raylib.h>
#include <algorithm>
#include <cmath>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include <glm/glm.hpp>
using namespace glm;

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

namespace {
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

    static const i32 HEX_NEIGHBOR_Q[6] = {
         0, -1, -1,  0,  1,  1
    };

    static const i32 HEX_NEIGHBOR_R[6] = {
         1,  1,  0, -1, -1,  0
    };

    static const vec2 HEX_FACE_NORMAL_2D[6] = {
        vec2( 0.5f,  0.86602540378443864676f),
        vec2(-0.5f,  0.86602540378443864676f),
        vec2(-1.0f,  0.0f),
        vec2(-0.5f, -0.86602540378443864676f),
        vec2( 0.5f, -0.86602540378443864676f),
        vec2( 1.0f,  0.0f),
    };

    static const f32vec3 HEX_FACE_NORMAL_3D[6] = {
        f32vec3(-0.5f, 0.0f, -0.86602540378443864676f),
        f32vec3( 0.5f, 0.0f, -0.86602540378443864676f),
        f32vec3( 1.0f, 0.0f, -0.0f),
        f32vec3( 0.5f, 0.0f,  0.86602540378443864676f),
        f32vec3(-0.5f, 0.0f,  0.86602540378443864676f),
        f32vec3(-1.0f, 0.0f, -0.0f),
    };

    inline vec2 HexAxialToWorld(f32 q, f32 r) {
        return vec2(
            HEX_SIZE * HEX_SQRT3 * (q + r * 0.5f),
            HEX_SIZE * 1.5f * r
        );
    }

    inline vec2 HexWorldVectorToAxial(vec2 v) {
        f32 r = v.y / (HEX_SIZE * 1.5f);
        f32 q = v.x / (HEX_SIZE * HEX_SQRT3) - r * 0.5f;
        return vec2(q, r);
    }

    inline HexCell HexRound(f32 q, f32 r) {
        f32 cube_x = q;
        f32 cube_z = r;
        f32 cube_y = -cube_x - cube_z;

        i32 rx = (i32)roundf(cube_x);
        i32 ry = (i32)roundf(cube_y);
        i32 rz = (i32)roundf(cube_z);

        f32 x_diff = fabsf((f32)rx - cube_x);
        f32 y_diff = fabsf((f32)ry - cube_y);
        f32 z_diff = fabsf((f32)rz - cube_z);

        if (x_diff > y_diff && x_diff > z_diff) {
            rx = -ry - rz;
        } else if (y_diff > z_diff) {
            ry = -rx - rz;
        } else {
            rz = -rx - ry;
        }

        return HexCell{ rx, rz };
    }

    struct HexStepResult {
        f32 t;
        i32 edge;
        i32 next_q;
        i32 next_r;
        bool vertex_tie;

        i32 edge_count;
        i32 edges[6];
    };

    inline bool HexRayStep(
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
            } else if (fabsf(t - best_t) <= TIE_EPS && best_count < 6) {
                best_edges[best_count++] = edge;
            }
        }

        if (best_count <= 0 || !std::isfinite(best_t)) {
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

    inline void ResolveVisibleHexTie(
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

    inline f32vec3 HexFaceNormalFromEdge(i32 edge) {
        return HEX_FACE_NORMAL_3D[edge];
    }

    inline Rectangle PauseScreenRect(f32 screen_w, f32 screen_h) {
        return Rectangle{
            screen_w * 0.25f,
            screen_h * 0.25f,
            screen_w * 0.5f,
            screen_h * 0.5f
        };
    }

    inline Rectangle PauseContinueButton(Rectangle pause_screen) {
        return Rectangle{pause_screen.x + 40.0f, pause_screen.y + 120.0f, pause_screen.width - 80.0f, 60.0f};
    }

    inline Rectangle PauseExitButton(Rectangle pause_screen) {
        Rectangle continue_button = PauseContinueButton(pause_screen);
        return Rectangle{pause_screen.x + 40.0f, continue_button.y + 120.0f, pause_screen.width - 80.0f, 60.0f};
    }

    inline bool CursorCaptured() {
#if defined(PLATFORM_WEB)
        return EM_ASM_INT({
            return document.pointerLockElement ? 1 : 0;
        }) != 0;
#else
        return IsCursorHidden();
#endif
    }

    inline void StartPause(Game& game) {
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

    inline void StartUnpause(Game& game) {
        game.unpause_delay_frames = UNPAUSE_DELAY_FRAMES;
        game.pause_cursor_released = false;
        game.pause_blocked_after_unpause = true;
        game.cursor_hidden_after_unpause_frames = 0;
        game.mouse_motion = vec2(0.0f);
        DisableCursor();
    }

    inline vec2 SmoothMouseMotion(Game& game, Vector2 raw_delta) {
        vec2 incoming_motion(
            std::clamp(raw_delta.x, -MAX_MOUSE_DELTA_PER_FRAME, MAX_MOUSE_DELTA_PER_FRAME),
            std::clamp(raw_delta.y, -MAX_MOUSE_DELTA_PER_FRAME, MAX_MOUSE_DELTA_PER_FRAME)
        );

        game.mouse_motion = mix(game.mouse_motion, incoming_motion, MOUSE_MOTION_MIX);

        if (fabsf(game.mouse_motion.x) < MOUSE_DELTA_EPSILON) {
            game.mouse_motion.x = 0.0f;
        }
        if (fabsf(game.mouse_motion.y) < MOUSE_DELTA_EPSILON) {
            game.mouse_motion.y = 0.0f;
        }

        return game.mouse_motion;
    }
}

void InGameState::OnLoad(CurrentGameInfo& info) {
    /*
    struct LevelShape {
        u32 color = 0xffcc00ff;
        i32 x1, z1, x2, z2;
        HeightModifier modifier = HeightModifier::MAX;
        f32 height = 0.0f;
    };
    */
    game = Game(Level(1024, 1024));
    game.BuildLevel({
        LevelShape{
            .x1 = 512 - 25, .z1 = 512 + 25,
            .x2 = 512 + 25, .z2 = 512 + 50,
            .height = 10.0f
        },
        LevelShape{
            .color = 0xff0000,
            .x1 = 512 - 15, .z1 = 512 + 15,
            .x2 = 512 + 15, .z2 = 512 + 25,
            .height = 5.0f
        }
    });
    game.camera.y = 5.0f;

    auto playerComp = EntityComponent{};
    playerComp.pos.x = (float)game.level.width / 2.0f;
    playerComp.pos.z = (float)game.level.length / 2.0f;
    info.flecs->entity("Player").set<EntityComponent>(playerComp).set<PlayerComponent>(PlayerComponent{});
}

void InGameState::OnUpdate(CurrentGameInfo& info) {
    auto& level = game.level;
    game.time += 1.0f;
    game.nausea = 2.0f;

    if (!game.pause_blocked_after_unpause && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))) {
        if (game.paused && game.unpause_delay_frames == 0) {
            StartUnpause(game);
        } else if (!game.paused) {
            StartPause(game);
        }
    }

    if (game.pause_blocked_after_unpause) {
        DisableCursor();

        if (CursorCaptured()) {
            game.cursor_hidden_after_unpause_frames++;
            if (game.cursor_hidden_after_unpause_frames >= POST_UNPAUSE_HIDDEN_FRAMES) {
                game.pause_blocked_after_unpause = false;
            }
        } else {
            game.cursor_hidden_after_unpause_frames = 0;
        }
    } else if (!game.paused && !CursorCaptured()) {
        StartPause(game);
    }

    if (game.unpause_delay_frames > 0) {
        DisableCursor();
        game.unpause_delay_frames--;

        if (game.unpause_delay_frames == 0) {
            game.paused = false;
            game.pause_cursor_released = false;
        }
    } else if (!game.paused) {
        if (!CursorCaptured()) {
            DisableCursor();
        }
    }

    if (game.paused && game.unpause_delay_frames == 0) {
        auto mousePos = GetMousePosition();
        Rectangle pause_screen = PauseScreenRect((f32)screenWidth, (f32)screenHeight);
        Rectangle continue_button = PauseContinueButton(pause_screen);
        Rectangle exit_button = PauseExitButton(pause_screen);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mousePos, continue_button)) {
                StartUnpause(game);
                return;
            }

            if (CheckCollisionPointRec(mousePos, exit_button)) {
                info.gameStateController->LoadGameState(info, std::make_shared<MainMenuState>());
                return;
            }
        }
    }

    vec2 mouseMotion(0.0f);
    if (game.paused || game.unpause_delay_frames > 0 || game.pause_blocked_after_unpause || !CursorCaptured()) {
        game.mouse_motion = vec2(0.0f);
    } else {
        mouseMotion = SmoothMouseMotion(game, GetMouseDelta());
    }

    const f32 fov = 90.0f;
    const f32 fov_rad = radians(fov);
    const f32 half_width = tanf(fov_rad * 0.5f);

    f32 yaw = glm::radians(game.yaw);

    f32 forward_x = sinf(yaw);
    f32 forward_z = cosf(yaw);

    f32 right_x = cosf(yaw);
    f32 right_z = -sinf(yaw);

    const float MIX_VAL = 0.2f;

    info.flecs->each([&](EntityComponent& entity, PlayerComponent& player) {
        if (game.paused) { // don't control player if paused
            return;
        }

        const vec2 mouse_sensitivity(0.250f, 0.125f);
        vec2 look_motion = mouseMotion * mouse_sensitivity;
        entity.pitch -= look_motion.y;
        entity.yaw += look_motion.x;
        entity.pitch = clamp(entity.pitch, -90.0f, 90.0f);

        if (IsKeyDown(KEY_W)) {
            entity.walk_x += forward_x;
            entity.walk_z += forward_z;
        }
        if (IsKeyDown(KEY_S)) {
            entity.walk_x -= forward_x;
            entity.walk_z -= forward_z;
        }
        if (IsKeyDown(KEY_D)) {
            entity.walk_x += right_x;
            entity.walk_z += right_z;
        }
        if (IsKeyDown(KEY_A)) {
            entity.walk_x -= right_x;
            entity.walk_z -= right_z;
        }
        if (IsKeyDown(KEY_SPACE)) {
            entity.jumping = true;
        } else {
            entity.jumping = false;
        }

        auto vec = vec2(entity.walk_x, entity.walk_z);
        f32 walk_len2 = dot(vec, vec);
        if (walk_len2 > 0.0f) {
            vec *= 1.0f / sqrtf(walk_len2);
        }
        entity.walk_x = vec.x;
        entity.walk_z = vec.y;

        game.camera.x = entity.pos.x;
        game.camera.y = entity.pos.y + entity.height;
        game.camera.z = entity.pos.z;
        game.pitch = entity.pitch;
        game.yaw = entity.yaw;
    });

    if (game.paused) {
        return; // pause all entity updates
    }

    info.flecs->each([&](EntityComponent& entity) {
        auto projected_pos = entity.pos + entity.velocity;

        HexCell player_cell = HexRound(projected_pos.x, projected_pos.z);

        int px = player_cell.q;
        int pz = player_cell.r;

        auto height = level.AtConst(px, pz);
        if (height > projected_pos.y + entity.step_height) {
            auto normal = level.NormalAt(px, pz);
            projected_pos.x -= normal.x;
            projected_pos.z -= normal.z;
            entity.velocity.x -= normal.x;
            entity.velocity.z -= normal.z;
            height = level.AtConst(px, pz);

            if (glm::length(normal) == 0.0f) {
                entity.pos = projected_pos;
                if (height > entity.pos.y) {
                    entity.pos.y += 0.25f;
                    if (entity.pos.y > height) {
                        entity.pos.y = height;
                    }
                }
            }
        } else {
            entity.pos = projected_pos;
            if (height > entity.pos.y) {
                entity.pos.y += 0.25f;
                if (entity.pos.y > height) {
                    entity.pos.y = height;
                }
            }
        }

        if (entity.pos.y <= height) {
            entity.on_ground = true;
            if (entity.velocity.y < 0) {
                entity.velocity.y = 0;
            }
        } else {
            entity.on_ground = false;
            entity.velocity.y += game.gravity;
        }

        if (entity.on_ground) {
            auto axial_walk = HexWorldVectorToAxial({entity.walk_x, entity.walk_z});
            entity.velocity.x = mix(entity.velocity.x, axial_walk.x, MIX_VAL);
            entity.velocity.z = mix(entity.velocity.z, axial_walk.y, MIX_VAL);
            if (entity.jumping) {
                entity.velocity.y = 1.5f;
                entity.jumping = false;
            }
        }

        entity.walk_x = 0.0f;
        entity.walk_z = 0.0f;
    });
}

void InGameState::OnRender(CurrentGameInfo& info) {
    auto& level = game.level;

    const f32 fov = 90.0f;
    const f32 fov_rad = radians(fov);
    const f32 half_width = tanf(fov_rad * 0.5f);

    f32 yaw = glm::radians(game.yaw);

    f32 forward_x = sinf(yaw);
    f32 forward_z = cosf(yaw);

    f32 right_x = cosf(yaw);
    f32 right_z = -sinf(yaw);

    const i32 screen_w = 720;
    const i32 screen_h_i = 720;
    const f32 screen_w_f = 720.0f;
    const f32 screen_h = 720.0f;
    const f32 screen_center_y = 360.0f + game.pitch * 10.0f;
    const f32 vertical_fov = glm::radians(90.0f);
    const f32 projection_scale = (screen_h * 0.5f) / tanf(vertical_fov * 0.5f);

    const i32 view_dist = 100;
    const f32 inv_view_dist = 1.0f / (f32)view_dist;

    f32vec3 sky_color = f32vec3(0.2f, 0.5f, 1.0f);
    Color sky_draw_color = VecToColor(sky_color);

    DrawRectangle(0, 0, screen_w, screen_h_i, sky_draw_color);

    vec2 ray_origin = HexAxialToWorld(game.camera.x, game.camera.z);
    vec2 forward_world = vec2(forward_x, forward_z);
    vec2 right_world = vec2(forward_world.y, -forward_world.x);

    HexCell start_cell = HexRound(game.camera.x, game.camera.z);

    const f32vec3 sun_dir = normalize(f32vec3(1, 1, 1));
    const bool use_nausea = game.nausea != 0.0f;
    const f32 nausea_time = game.time * 0.05f;

    for (int i = 0; i < screen_w; i++) {
        f32 screen_x = ((i + 0.5f) / screen_w_f) * 2.0f - 1.0f;
        f32 camera_x = screen_x * half_width;

        vec2 ray_dir = forward_world + right_world * camera_x;

        int map_x = start_cell.q;
        int map_z = start_cell.r;

        int y_top = screen_h_i;
        f32 depth = 0.0f;

        f32 prev_height = 0.0f;
        if (level.InBounds(map_x, map_z)) {
            prev_height = level.At(map_x, map_z);
        }

        int nausea_offset = 0;
        if (use_nausea) {
            nausea_offset = (int)(game.nausea * glm::sin((float)i * 0.01f + nausea_time) * 15.0f);
        }

        auto ProjectY = [&](f32 world_y, f32 d) -> int {
            f32 safe_d = std::max(d, 0.001f);
            f32 projected =
                screen_center_y -
                ((world_y - game.camera.y) / safe_d) * projection_scale;

            return (int)projected + nausea_offset;
        };

        auto CameraDepthFromHexT = [](f32 t) -> f32 {
            return t;
        };
        // Render Terrain
        while (depth < (f32)view_dist) {
            f32 old_height = prev_height;

            HexStepResult enter_step{};
            if (!HexRayStep(map_x, map_z, ray_origin, ray_dir, depth + STEP_EPS, enter_step)) {
                break;
            }

            ResolveVisibleHexTie(enter_step, map_x, map_z, old_height, level);

            f32 cell_enter = enter_step.t;
            depth = cell_enter;

            map_x = enter_step.next_q;
            map_z = enter_step.next_r;

            if (!level.InBounds(map_x, map_z)) {
                prev_height = 0.0f;
                continue;
            }

            f32 world_height = level.At(map_x, map_z);
            prev_height = world_height;

            HexStepResult exit_step{};
            f32 cell_exit = cell_enter + 1.0f;
            if (HexRayStep(map_x, map_z, ray_origin, ray_dir, cell_enter + STEP_EPS, exit_step)) {
                cell_exit = exit_step.t;
            }

            auto hit_color = U32ToVec(level.ColorAt(map_x, map_z));

            f32 dist_norm = std::clamp(cell_enter * inv_view_dist, 0.0f, 1.0f);
            hit_color = mix(hit_color, sky_color, dist_norm);

            if (world_height > old_height) {
                f32 enter_depth = CameraDepthFromHexT(cell_enter);

                int y_top_wall = ProjectY(world_height, enter_depth);
                int y_bottom_wall = ProjectY(old_height, enter_depth);

                int draw_top = std::clamp(y_top_wall, 0, screen_h_i);
                int draw_bottom = std::clamp(y_bottom_wall, 0, screen_h_i);

                draw_bottom = std::min(draw_bottom, y_top);

                if (draw_top < draw_bottom) {
                    f32vec3 wall_normal = HexFaceNormalFromEdge(enter_step.edge);

                    f32 sunlight = (dot(wall_normal, sun_dir) + 3.0f) * 0.25f;
                    auto color = VecToColor(mix(hit_color * sunlight, sky_color, dist_norm));

                    DrawLine(i, draw_bottom, i, draw_top, color);

                    y_top = draw_top;

                    if (y_top <= 0) {
                        break;
                    }
                }
            }

            if (world_height < game.camera.y) {
                f32 enter_depth = CameraDepthFromHexT(cell_enter);
                f32 exit_depth  = CameraDepthFromHexT(cell_exit);

                int y_near = ProjectY(world_height, enter_depth);
                int y_far  = ProjectY(world_height, exit_depth);

                int draw_top = std::clamp(std::min(y_near, y_far), 0, screen_h_i);
                int draw_bottom = std::clamp(std::max(y_near, y_far), 0, screen_h_i);

                draw_bottom = std::min(draw_bottom, y_top);

                if (draw_top < draw_bottom) {
                    f32vec3 top_normal = level.NormalAt(map_x, map_z);

                    f32 sunlight = (dot(top_normal, sun_dir) + 3.0f) * 0.25f;
                    auto color = VecToColor(mix(hit_color * sunlight, sky_color, dist_norm));

                    DrawLine(i, draw_bottom, i, draw_top, color);

                    y_top = draw_top;

                    if (y_top <= 0) {
                        break;
                    }
                }
            }
        }
    }

    if (game.paused) {
        auto mousePos = GetMousePosition();
        Rectangle pause_screen = PauseScreenRect(screen_w_f, screen_h);
        DrawRectangle(0, 0, screen_w, screen_h_i, Color{0, 0, 0, 120});
        DrawRectangleRec(pause_screen, GRAY);
        DrawText("Paused", (int)pause_screen.x + 95, (int)pause_screen.y + 30, 48, BLACK);

        Rectangle continue_button = PauseContinueButton(pause_screen);
        Color button_color = GREEN;
        if (CheckCollisionPointRec(mousePos, continue_button)) {
            button_color = DARKGREEN;
        }

        DrawRectangleRec(continue_button, button_color);
        DrawText("Continue", (int)continue_button.x + 35, (int)continue_button.y + 10, 40, BLACK);

        Rectangle exit_button = PauseExitButton(pause_screen);
        button_color = GREEN;
        if (CheckCollisionPointRec(mousePos, exit_button)) {
            button_color = DARKGREEN;
        }

        DrawRectangleRec(exit_button, button_color);
        DrawText("Exit", (int)exit_button.x + 35, (int)exit_button.y + 10, 40, BLACK);
    }
}

void InGameState::OnUnload(CurrentGameInfo& info) {

}
