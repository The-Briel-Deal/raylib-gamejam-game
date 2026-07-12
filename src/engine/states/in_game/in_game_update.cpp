#include "glm/geometric.hpp"
#include "in_game_common.h"
#include "../game_state_controller.h"
#include "../../current_game_info.h"

using namespace glm;
using namespace in_game;

void InGameState::OnUpdate(CurrentGameInfo& info) {
    auto& level = game.level;
    game.time += 1.0f;
    //game.nausea = 2.0f;

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

    vec2 mouseMotion(0.0f);
    if (game.paused || game.unpause_delay_frames > 0 || game.pause_blocked_after_unpause || !CursorCaptured()) {
        game.mouse_motion = vec2(0.0f);
    } else {
        mouseMotion = SmoothMouseMotion(game, GetMouseDelta());
    }

    const f32 fov = 90.0f;
    const f32 fov_rad = radians(fov);
    const f32 half_width = glm::tan(fov_rad * 0.5f);

    f32 yaw = glm::radians(game.yaw);

    f32 forward_x = glm::sin(yaw);
    f32 forward_z = glm::cos(yaw);

    f32 right_x = glm::cos(yaw);
    f32 right_z = -glm::sin(yaw);

    const float MIX_VAL = 0.2f;

    EntityComponent mainPlayer; // only built for singleplayer currently.  Could switch to a vector for multiplayer but there's no time :(

    info.flecs->each([&](EntityComponent& entity, PlayerComponent& player) {
        if (game.paused) { // don't control player if paused
            return;
        }

        mainPlayer = entity;

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
            vec *= 1.0f / glm::sqrt(walk_len2);
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

    info.flecs->each([&](flecs::entity e, EntityComponent& entity, SkeletonComponent& skeleton, VisibleEntityComponent& visible) {
        visible.has_texture = true;
        switch (skeleton.state) {

        case SkeletonState::IDLE:
            visible.texture = info.skeletonInfo.idle;
            if (skeleton.timer > 3) {
                skeleton.timer = 0;
                skeleton.state = SkeletonState::SEARCHING;
            }
            break;
        case SkeletonState::SEARCHING:
            {
                int frame = ((int)skeleton.timer) & 1;
                visible.texture = info.skeletonInfo.look[frame];

                if (skeleton.timer > 2 && glm::distance(mainPlayer.pos, entity.pos) <= 15) {
                    skeleton.timer = 0;
                    skeleton.state = SkeletonState::FOUND;
                }

                if (skeleton.timer > 4) {
                    skeleton.timer = 0;
                    skeleton.state = SkeletonState::IDLE;
                }
            }
            
            break;
        case SkeletonState::FOUND:
            visible.texture = info.skeletonInfo.shock;
            skeleton.is_holding = false;
            skeleton.held = false;
            skeleton.reload_timer = 0;
            if (skeleton.timer > 3) {
                skeleton.state = SkeletonState::CHASING;
            }
            break;
        case SkeletonState::CHASING:
            {
                float closest_skele = 1000;
                flecs::entity skele;
                EntityComponent& sk_e = entity;
                SkeletonComponent& sk = skeleton;
                
                if (skeleton.is_holding == false && skeleton.held == false) {
                    info.flecs->each([&](flecs::entity e2, EntityComponent& other, SkeletonComponent& other_skele) {
                        if (e.name() != e2.name()) {
                            if (other_skele.is_holding || other_skele.held) {
                                return;
                            }
                            float d = glm::distance(entity.pos, other.pos);
                            if (d < closest_skele) {
                                if (other_skele.state == SkeletonState::CHASING) {
                                    closest_skele = d;
                                    skele = e2;
                                    sk_e = other;
                                    sk = other_skele;
                                }
                            }
                        }
                    });
                }

                if (closest_skele <= 10) {
                    int frame = (int)(skeleton.timer * 4) % 6;
                    visible.texture = info.skeletonInfo.running[frame];
                    vec3 move = glm::normalize(mainPlayer.pos - sk_e.pos);
                    entity.walk_x = move.x;
                    entity.walk_z = move.z;

                    if (closest_skele <= 1) {
                        skeleton.is_holding = true;
                        sk.held = true;
                        skeleton.holding = skele;
                        sk.state = SkeletonState::SITTING;
                        skeleton.state = SkeletonState::CARRYING_CHASING;
                    }

                } else {
                    float dist = glm::distance(mainPlayer.pos, entity.pos);
                    if (dist <= 8) {
                        if (skeleton.reload_timer <= 0) {
                            skeleton.state = SkeletonState::EQUIP_PISTOL;
                            skeleton.timer = 0;
                        } else {
                            visible.texture = info.skeletonInfo.idle;
                        }
                    } else {
                        int frame = (int)(skeleton.timer * 4) % 6;
                        visible.texture = info.skeletonInfo.running[frame];
                        vec3 move = glm::normalize(mainPlayer.pos - entity.pos);
                        entity.walk_x = move.x;
                        entity.walk_z = move.z;
                    }
                    
                }

                
            }
            break;
        case SkeletonState::SITTING:
            visible.texture = info.skeletonInfo.sit;
            break;
        case SkeletonState::CARRYING_IDLE:
            break;
        case SkeletonState::CARRYING_CHASING:
            {
                auto e = skeleton.holding;
                auto& ent = e.get_mut<EntityComponent>();
                ent.pos = entity.pos + glm::vec3(0,  3.0f, 0);
                auto& sk = e.get_mut<SkeletonComponent>();
                sk.state = SkeletonState::SITTING;

                ent.velocity = {};
                ent.on_ground = true;

                float dist = glm::distance(mainPlayer.pos, entity.pos);
                if (dist <= 5) {
                    visible.texture = info.skeletonInfo.hold;
                    skeleton.timer = 0;
                    skeleton.state = SkeletonState::IDLE;
                    skeleton.is_holding = false;
                    sk.held = false;
                    sk.state = SkeletonState::FLYING;
                    sk.timer = 0;

                    vec3 move = glm::normalize(mainPlayer.pos - entity.pos);
                    sk.fly_vec = {move.x, move.z};
                } else {
                    int frame = (int)(skeleton.timer * 4) % 6;
                    visible.texture = info.skeletonInfo.hold_running[frame];
                    vec3 move = glm::normalize(mainPlayer.pos - entity.pos);
                    entity.walk_x = move.x;
                    entity.walk_z = move.z;
                }
            }
            break;
        case SkeletonState::THROWING:
            break;
        case SkeletonState::FLYING:
            {
                skeleton.held = false;
                skeleton.is_holding = false;
                vec3 move = {skeleton.fly_vec.x, 0, skeleton.fly_vec.y};
                entity.velocity.x = move.x;
                entity.velocity.z = move.z;
                entity.velocity.y *= 0.1f;
                if (skeleton.timer > 1) {
                    skeleton.state = SkeletonState::CHASING;   
                }
            }
            break;
        case SkeletonState::EQUIP_PISTOL:
            {
                int frame = (int)(skeleton.timer * 6);
                if (frame >= 17) {
                    visible.texture = info.skeletonInfo.pistol;
                    skeleton.timer = 0;
                    skeleton.state = SkeletonState::AIM;
                } else {
                    visible.texture = info.skeletonInfo.pistol_pull[frame];
                }
            }
            break;
        case SkeletonState::AIM:
            visible.texture = info.skeletonInfo.pistol;
            if (skeleton.timer >= 1.5f) {
                skeleton.state = SkeletonState::SHOOT;
                skeleton.timer = 0;
            }
            break;
        case SkeletonState::SHOOT:
            int frame = (int)(skeleton.timer * 5) % 5;
            visible.texture = info.skeletonInfo.shoot[frame];
            if (skeleton.timer * 5 >= 5 * 3) {
                skeleton.timer = 0;
                skeleton.reload_timer = 5;
                skeleton.state = SkeletonState::CHASING;
            }
            break;
        }
        skeleton.timer += info.frame_delta;
        if (skeleton.reload_timer > 0) {
            skeleton.reload_timer -= info.frame_delta;
        }
    });

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
