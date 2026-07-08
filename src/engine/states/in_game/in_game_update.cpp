#include "in_game_common.h"

using namespace glm;
using namespace in_game;

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
