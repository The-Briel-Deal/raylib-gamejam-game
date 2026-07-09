#include "game.h"
#include <utility>
#include "../../current_game_info.h"
#include "../in_game/in_game_common.h"
#include "raylib.h"

using namespace in_game;

Game::Game(Level level) : level(std::move(level)) {
    camera.x = (f32)this->level.width / 2.0f;
    camera.z = (f32)this->level.length / 2.0f;
}

void Game::BuildLevel(std::vector<LevelShape> shapes) {
    for (auto& shape : shapes) {
        
        for (int x = shape.x1; x < shape.x2; x++) {
            for (int z = shape.z1; z < shape.z2; z++) {
                if (level.InBounds(x, z)) {
                    auto& height = level.At(x, z);
                    auto& color = level.ColorAt(x, z);

                    if (shape.modifier == HeightModifier::MIN) {
                        if (height >= shape.height) {
                            height = shape.height;
                            color = shape.color;
                        }
                    } else {
                        if (height <= shape.height) {
                            height = shape.height;
                            color = shape.color;
                        }
                    }
                }
            }
        }
    }
}

void Game::OnRender(CurrentGameInfo& info) {
    auto mousePos = GetMousePosition();
    auto iMouse = i32vec2((int)mousePos.x, (int)mousePos.y);
    const f32 fov = 90.0f;
    const f32 fov_rad = radians(fov);
    const f32 half_width = glm::tan(fov_rad * 0.5f);

    f32 yaw = glm::radians(this->yaw);

    f32 forward_x = glm::sin(yaw);
    f32 forward_z = glm::cos(yaw);

    f32 right_x = glm::cos(yaw);
    f32 right_z = -glm::sin(yaw);

    const i32 screen_w = 720;
    const i32 screen_h_i = 720;
    const f32 screen_w_f = 720.0f;
    const f32 screen_h = 720.0f;
    const f32 screen_center_y = 360.0f + pitch * 10.0f;
    const f32 vertical_fov = glm::radians(90.0f);
    const f32 projection_scale = (screen_h * 0.5f) / glm::tan(vertical_fov * 0.5f);

    const i32 view_dist = 100;
    const f32 inv_view_dist = 1.0f / (f32)view_dist;

    f32vec3 sky_color = f32vec3(0.2f, 0.5f, 1.0f);

    vec2 ray_origin = HexAxialToWorld(camera.x, camera.z);
    vec2 forward_world = vec2(forward_x, forward_z);
    vec2 right_world = vec2(forward_world.y, -forward_world.x);

    HexCell start_cell = HexRound(camera.x, camera.z);

    const f32vec3 sun_dir = normalize(f32vec3(1, 1, 1));
    const bool use_nausea = nausea != 0.0f;
    const f32 nausea_time = time * 0.05f;

    select_x = -1;
    select_z = -1;

    bool hit = false;
    
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
            nausea_offset = (int)(nausea * glm::sin((float)i * 0.01f + nausea_time) * 15.0f);
        }

        auto ProjectY = [&](f32 world_y, f32 d) -> int {
            f32 safe_d = glm::max(d, 0.001f);
            f32 projected =
                screen_center_y -
                ((world_y - camera.y) / safe_d) * projection_scale;

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

            

            auto find = entity_buckets.find(map_x + map_z * (int)level.width);
            if (find != entity_buckets.end()) {
                auto& vec = find->second;
                for (auto& check : vec) {
                    check.visible = true;
                    
                    if (check.entity.has<VisibleEntityComponent>()) {
                        auto& comp = check.entity.get_mut<VisibleEntityComponent>();

                        Rectangle rect{};
                        auto& game = *this;
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

            f32 dist_norm = glm::clamp(cell_enter * inv_view_dist, 0.0f, 1.0f);
            hit_color = mix(hit_color, sky_color, dist_norm);



            if (world_height > old_height) {
                f32 enter_depth = CameraDepthFromHexT(cell_enter);

                int y_top_wall = ProjectY(world_height, enter_depth);
                int y_bottom_wall = ProjectY(old_height, enter_depth);

                int draw_top = glm::clamp(y_top_wall, 0, screen_h_i);
                int draw_bottom = glm::clamp(y_bottom_wall, 0, screen_h_i);

                draw_bottom = glm::min(draw_bottom, y_top);

                if (draw_top < draw_bottom) {
                    f32vec3 wall_normal = HexFaceNormalFromEdge(enter_step.edge);

                    f32 sunlight = (dot(wall_normal, sun_dir) + 3.0f) * 0.25f;
                    auto color = VecToColor(mix(hit_color * sunlight, sky_color, dist_norm));

                    bool selected = false;

                    if (i == iMouse.x) {
                        if (iMouse.y <= draw_bottom && iMouse.y >= draw_top) {
                            if (!hit) {
                                selected = true;
                                hit = true;
                                select_x = map_x;
                                select_z = map_z;
                            }
                        }
                    }
                    if (selected) {
                        DrawLine(i, draw_bottom, i, draw_top, RED);
                    }
                    else {
                        DrawLine(i, draw_bottom, i, draw_top, color);
                    }

                    y_top = draw_top;

                    if (y_top <= 0) {
                        break;
                    }
                }
            }

            if (world_height < camera.y) {
                f32 enter_depth = CameraDepthFromHexT(cell_enter);
                f32 exit_depth  = CameraDepthFromHexT(cell_exit);

                int y_near = ProjectY(world_height, enter_depth);
                int y_far  = ProjectY(world_height, exit_depth);

                int draw_top = glm::clamp(glm::min(y_near, y_far), 0, screen_h_i);
                int draw_bottom = glm::clamp(glm::max(y_near, y_far), 0, screen_h_i);

                draw_bottom = glm::min(draw_bottom, y_top);

                if (draw_top < draw_bottom) {
                    f32vec3 top_normal = level.NormalAt(map_x, map_z);

                    f32 sunlight = (dot(top_normal, sun_dir) + 3.0f) * 0.25f;
                    auto color = VecToColor(mix(hit_color * sunlight, sky_color, dist_norm));

                    
                    bool selected = false;

                    if (i == iMouse.x) {
                        if (iMouse.y <= draw_bottom && iMouse.y >= draw_top) {
                            if (!hit) {
                                selected = true;
                                hit = true;
                                select_x = map_x;
                                select_z = map_z;
                            }
                        }
                    }
                    if (selected) {
                        DrawLine(i, draw_bottom, i, draw_top, RED);
                    }
                    else {
                        DrawLine(i, draw_bottom, i, draw_top, color);
                    }

                    y_top = draw_top;

                    if (y_top <= 0) {
                        break;
                    }
                }
            }
        }
    }
}
