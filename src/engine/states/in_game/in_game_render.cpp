#include "in_game_common.h"

using namespace glm;
using namespace in_game;

namespace in_game {
bool ProjectEntityRect(
    const EntityComponent& entity,
    const Game& game,
    Rectangle& out_rect
) {
    constexpr float screen_w = 720.0f;
    constexpr float screen_h = 720.0f;
    constexpr float fov = 90.0f;

    float fov_rad = glm::radians(fov);
    float half_width = tanf(fov_rad * 0.5f);
    float projection_scale = (screen_h * 0.5f) / tanf(glm::radians(90.0f) * 0.5f);

    float yaw = glm::radians(game.yaw);
    vec2 forward_world(sinf(yaw), cosf(yaw));
    vec2 right_world(forward_world.y, -forward_world.x);

    vec2 cam_world = HexAxialToWorld(game.camera.x, game.camera.z);
    vec2 ent_world = HexAxialToWorld(entity.pos.x, entity.pos.z);
    vec2 rel = ent_world - cam_world;

    float depth = dot(rel, forward_world);
    if (depth <= 0.01f) return false;

    float side = dot(rel, right_world);
    float camera_x = side / depth;

    if (fabsf(camera_x) > half_width) return false;

    float screen_x = ((camera_x / half_width) * 0.5f + 0.5f) * screen_w;

    float screen_center_y = 360.0f + game.pitch * 10.0f;

    auto project_y = [&](float world_y) {
        return screen_center_y - ((world_y - game.camera.y) / depth) * projection_scale;
    };

    float y_top = project_y(entity.pos.y + entity.height);
    float y_bottom = project_y(entity.pos.y);

    float half_rect_width = (entity.radius / depth) * projection_scale;

    out_rect = Rectangle{
        screen_x - half_rect_width,
        y_top,
        half_rect_width * 2.0f,
        y_bottom - y_top
    };

    return out_rect.width > 0.0f && out_rect.height > 0.0f;
}
}

void InGameState::OnRender(CurrentGameInfo& info) {
    auto& level = game.level;

    game.entity_buckets.clear();

    info.flecs->each([&](flecs::entity entity, EntityComponent& comp) {
        
        auto e_cell = HexRound(comp.pos.x, comp.pos.z);
        int key = e_cell.q + e_cell.r * (int)game.level.width;
        auto find = game.entity_buckets.find(key);
        if (find == game.entity_buckets.end()) {
            game.entity_buckets[key] = {};
        }
        game.entity_buckets[key].push_back(EntityVisibilityCheck{
            .entity = entity,
            .visible = false
        });
    });

    info.flecs->each([&](VisibleEntityComponent& comp, EntityComponent& entity) {
        comp.visible = false;
        Rectangle rect{};
        if (ProjectEntityRect(entity, game, rect)) {
            comp.clip_x0 = rect.x;
            comp.clip_y0 = rect.y;
            comp.clip_x1 = rect.x + rect.width;
            comp.clip_y1 = rect.y + rect.height;
        }
    });

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

            

            auto find = game.entity_buckets.find(map_x + map_z * (int)game.level.width);
            if (find != game.entity_buckets.end()) {
                auto& vec = find->second;
                for (auto& check : vec) {
                    check.visible = true;
                    
                    if (check.entity.has<VisibleEntityComponent>()) {
                        auto& comp = check.entity.get_mut<VisibleEntityComponent>();

                        Rectangle rect{};
                        if (ProjectEntityRect(check.entity.get<EntityComponent>(), game, rect)) {
                            if (rect.y <= y_top) {
                                check.visible = true;
                                comp.visible = true;
                            } else {
                                if (i > comp.clip_x0 && i < rect.x + rect.width / 2) {
                                    comp.clip_x0 = i;
                                }
                                if (i < comp.clip_x1 && i > rect.x + rect.width / 2) {
                                    comp.clip_x1 = i;
                                }
                            }
                            if (y_top < comp.clip_y1) {
                                //comp.clip_y1 = y_top;
                            }
                        }
                    }
                    
                }
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

    info.flecs->each([&](VisibleEntityComponent& visible, EntityComponent& entity) {
        if (visible.visible) {
            int nausea_offset = 0;
            if (use_nausea) {
                //nausea_offset = (int)(game.nausea * glm::sin((float)i * 0.01f + nausea_time) * 15.0f);
            }

            auto ProjectY = [&](f32 world_y, f32 d) -> int {
                f32 safe_d = std::max(d, 0.001f);
                f32 projected =
                    screen_center_y -
                    ((world_y - game.camera.y) / safe_d) * projection_scale;

                return (int)projected + nausea_offset;
            };

            

            Rectangle rect{};
            if (ProjectEntityRect(entity, game, rect)) {
                rect = {visible.clip_x0, visible.clip_y0, (visible.clip_x1 - visible.clip_x0), (visible.clip_y1 - visible.clip_y0)};
                auto dist = distance(entity.pos, f32vec3(game.camera.x, game.camera.y, game.camera.z));

                auto dist_norm = clamp(dist / (float)view_dist, 0.0f, 1.0f);

                auto color_vec = mix(visible.color, sky_color, dist_norm);

                if (visible.has_texture) {
                    DrawTexturePro(visible.texture, {0, 0, (float)visible.texture.width, (float)visible.texture.height}, rect, {}, 0, VecToColor(color_vec));
                } else {
                    DrawRectangleRec(rect, VecToColor(color_vec));
                }
                
            }
        }
    });

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
