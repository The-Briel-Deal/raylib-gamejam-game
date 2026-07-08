#include "game/game.h"
#include "game_state_controller.h"
#include "../current_game_info.h"
#include <raylib.h>
#include <algorithm>
#include <execution>
#include <numeric>

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
    
}

void InGameState::OnRender(CurrentGameInfo& info) {

    auto& level = game.level;
    game.time += 1.0f;
    game.nausea = 1.0f;

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
        if (IsKeyDown(KEY_RIGHT)) {
            entity.yaw += 5;
        }
        if (IsKeyDown(KEY_LEFT)) {
            entity.yaw -= 5;
        }

        if (IsKeyDown(KEY_UP)) {
            entity.pitch += 5;
        }
        if (IsKeyDown(KEY_DOWN)) {
            entity.pitch -= 5;
        }
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

        if (IsKeyPressed(KEY_ESCAPE)) {
            EnableCursor();
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            DisableCursor();
        }

        auto vec = vec2(entity.walk_x, entity.walk_z);
        if (length(vec) > 0) {
            vec = normalize(vec);
        }
        entity.walk_x = vec.x;
        entity.walk_z = vec.y;

        game.camera.x = entity.pos.x;
        game.camera.y = entity.pos.y + entity.height;
        game.camera.z = entity.pos.z;
        game.pitch = entity.pitch;
        game.yaw = entity.yaw;
    });

    info.flecs->each([&](EntityComponent& entity) {
        
        auto projected_pos = entity.pos + entity.velocity;

        int px = (int)projected_pos.x;
        int pz = (int)projected_pos.z;

        auto height = level.AtConst(px, pz);
        if (height > projected_pos.y + entity.step_height) {
            auto normal = level.NormalAt(px, pz);
            projected_pos.x -= normal.x;
            projected_pos.z -= normal.z;
            entity.velocity.x -= normal.x;
            entity.velocity.z -= normal.z;
            height = level.AtConst(px, pz);
        } else {
            entity.pos = projected_pos;
            if (height > entity.pos.y) {
                entity.pos.y = height;
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
            entity.velocity.x = mix(entity.velocity.x, entity.walk_x, MIX_VAL);
            entity.velocity.z = mix(entity.velocity.z, entity.walk_z, MIX_VAL);
            if (entity.jumping) {
                entity.velocity.y = 1.5f;
                entity.jumping = false;
            }
        }
        
        entity.walk_x = 0.0f;
        entity.walk_z = 0.0f;
    });

    auto c_ix = (int)game.camera.x;
    auto c_iz = (int)game.camera.z;

    const f32 screen_h = 720.0f;
    const f32 vertical_fov = glm::radians(90.0f);
    const f32 projection_scale = (screen_h * 0.5f) / tanf(vertical_fov * 0.5f);

    const i32 view_dist = 50;

    f32vec3 sky_color = f32vec3(0.2f, 0.5f, 1.0f);

    DrawRectangle(0, 0, 720, 720, VecToColor(sky_color));

    for (int i = 0; i < 720; i++) {
        f32 screen_x = ((i + 0.5f) / 720.0f) * 2.0f - 1.0f;
        f32 camera_x = screen_x * half_width;

        f32 ray_x = forward_x + right_x * camera_x;
        f32 ray_z = forward_z + right_z * camera_x;

        int map_x = (int)floorf(game.camera.x);
        int map_z = (int)floorf(game.camera.z);

        int step_x;
        int step_z;

        f32 t_delta_x;
        f32 t_delta_z;

        f32 t_max_x;
        f32 t_max_z;

        if (ray_x > 0.0f) {
            step_x = 1;
            t_delta_x = 1.0f / ray_x;
            t_max_x = ((map_x + 1.0f) - game.camera.x) / ray_x;
        } else if (ray_x < 0.0f) {
            step_x = -1;
            t_delta_x = -1.0f / ray_x;
            t_max_x = (game.camera.x - map_x) / -ray_x;
        } else {
            step_x = 0;
            t_delta_x = INFINITY;
            t_max_x = INFINITY;
        }

        if (ray_z > 0.0f) {
            step_z = 1;
            t_delta_z = 1.0f / ray_z;
            t_max_z = ((map_z + 1.0f) - game.camera.z) / ray_z;
        } else if (ray_z < 0.0f) {
            step_z = -1;
            t_delta_z = -1.0f / ray_z;
            t_max_z = (game.camera.z - map_z) / -ray_z;
        } else {
            step_z = 0;
            t_delta_z = INFINITY;
            t_max_z = INFINITY;
        }

        int y_top = 720;

        f32 depth = 0.0f;

        int prev_x = map_x;
        int prev_z = map_z;

        f32 prev_height = 0.0f;
        if (level.InBounds(prev_x, prev_z)) {
            prev_height = level.At(prev_x, prev_z);
        }

        auto ProjectY = [&](f32 world_y, f32 d) -> int {
            f32 safe_d = std::max(d, 0.001f);
            f32 screen_center_y = 360.0f + game.pitch * 10.0f;

            f32 projected =
                screen_center_y -
                ((world_y - game.camera.y) / safe_d) * projection_scale;

            return (int)projected + (int)(game.nausea * glm::cos((float)i * 0.01f + game.time * 0.05f) * 15.0f);
        };

        while (depth < (f32)view_dist) {
            prev_x = map_x;
            prev_z = map_z;

            f32 old_height = prev_height;

            f32 cell_enter;

            if (t_max_x < t_max_z) {
                cell_enter = t_max_x;
                depth = t_max_x;
                t_max_x += t_delta_x;
                map_x += step_x;
            } else {
                cell_enter = t_max_z;
                depth = t_max_z;
                t_max_z += t_delta_z;
                map_z += step_z;
            }

            f32 cell_exit = std::min(t_max_x, t_max_z);

            if (!level.InBounds(map_x, map_z)) {
                prev_height = 0.0f;
                continue;
            }

            f32 world_height = level.At(map_x, map_z);
            prev_height = world_height;

            auto hit_color = U32ToVec(level.ColorAt(map_x, map_z));

            f32 dist_norm = std::clamp(cell_enter / (float)view_dist, 0.0f, 1.0f);
            hit_color = mix(hit_color, sky_color, dist_norm);

            auto sun_dir = normalize(f32vec3(1, 1, 1));

            if (world_height > old_height) {
                int y_top_wall = ProjectY(world_height, cell_enter);
                int y_bottom_wall = ProjectY(old_height, cell_enter);

                int draw_top = std::clamp(y_top_wall, 0, 720);
                int draw_bottom = std::clamp(y_bottom_wall, 0, 720);

                draw_bottom = std::min(draw_bottom, y_top);

                if (draw_top < draw_bottom) {
                    f32vec3 wall_normal = level.NormalAt(map_x, map_z);

                    f32 sunlight = (dot(wall_normal, sun_dir) + 3.0f) / 4.0f;
                    auto color = VecToColor(mix(hit_color * sunlight, sky_color, dist_norm));

                    DrawLine(i, draw_bottom, i, draw_top, color);

                    y_top = draw_top;

                    if (y_top <= 0) {
                        break;
                    }
                }
            }

            if (world_height < game.camera.y) {
                int y_near = ProjectY(world_height, cell_enter);
                int y_far  = ProjectY(world_height, cell_exit);

                int draw_top = std::clamp(std::min(y_near, y_far), 0, 720);
                int draw_bottom = std::clamp(std::max(y_near, y_far), 0, 720);

                draw_bottom = std::min(draw_bottom, y_top);

                if (draw_top < draw_bottom) {
                    f32vec3 top_normal = level.NormalAt(map_x, map_z);

                    f32 sunlight = (dot(top_normal, sun_dir) + 3.0f) / 4.0f;
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
}

void InGameState::OnUnload(CurrentGameInfo& info) {

}
