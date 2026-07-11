#include "game.h"
#include <chrono>
#include <utility>
#include "../../current_game_info.h"
#include "../in_game/in_game_common.h"
#include "raylib.h"
#include <rlgl.h>
#include <thread>

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

typedef struct {
    Vector2 start;
    Vector2 end;
    Color color;
} LineCommand;

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

    int SX = select_x;
    int SZ = select_z;
    select_x = -1;
    select_z = -1;

    bool hit = false;

    static std::vector<std::thread> threads;
    static std::vector<i32> threads_active;
    static std::vector<i32> threads_stopped;
    static std::vector<LineCommand> lines[4];

    if (!threads_loaded) {
        threads_loaded = true;
        // stop threads
        if (threads.size() > 0) {
            for (size_t t = 0; t < 4; t++) {
                threads_stopped[t] = true;
                if (threads[t].joinable()) {
                    threads[t].join();
                }
            }
            threads_stopped.clear();
            threads.clear();
            threads_active.clear();
        }
        for (size_t t = 0; t < 4; t++) {
            const size_t I = t;
            threads_active.push_back(false);
            threads_stopped.push_back(false);
            threads.push_back(std::thread([&, I]() {
                while (threads_stopped[I] == false) {
                     if (threads_active[I]) {
                        int div = 720 / 4;

                        lines[I].clear();
                        for (int i = (int)(I * div); i < (int)(I * div + div); i++) {
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

                                bool within_radius = (map_x - SX) * (map_x - SX) + (map_z - SZ) * (map_z - SZ) < select_radius * select_radius;

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
                                        
                                        auto cmd = LineCommand{};

                                        if (recolor_selected && within_radius) {
                                            cmd.color = {(unsigned char)(255 - color.r), (unsigned char)(255 - color.g), (unsigned char)(255 - color.b), color.a};
                                            cmd.start = {(float)i, (float)draw_bottom};
                                            cmd.end = {(float)i, (float)draw_top};
                                        } else {
                                            cmd.color = color;
                                            cmd.start = {(float)i, (float)draw_bottom};
                                            cmd.end = {(float)i, (float)draw_top};
                                        }
                                        lines[I].push_back(cmd);

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
                                        
                                        auto cmd = LineCommand{};

                                        if (recolor_selected && within_radius) {
                                            cmd.color = {(unsigned char)(255 - color.r), (unsigned char)(255 - color.g), (unsigned char)(255 - color.b), color.a};
                                            cmd.start = {(float)i, (float)draw_bottom};
                                            cmd.end = {(float)i, (float)draw_top};
                                        } else {
                                            cmd.color = color;
                                            cmd.start = {(float)i, (float)draw_bottom};
                                            cmd.end = {(float)i, (float)draw_top};
                                        }
                                        lines[I].push_back(cmd);

                                        y_top = draw_top;

                                        if (y_top <= 0) {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        threads_active[I] = false;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                threads_stopped[I] = false;
            }));
        }
    }

    for (size_t i = 0; i < 4; i++) {
        threads_active[i] = true;
    }

    bool threads_alive = true;

    while (threads_alive) {
        threads_alive = false;
        for (size_t i = 0; i < 4; i++) {
            if (threads_active[i] == true) {
                threads_alive = true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    for (size_t i = 0; i < 4; i++) {
        while (threads_active[i]) {
            
        }
    }
    
    rlBegin(RL_LINES);
    for (size_t l = 0; l < 4; l++) {
        for (size_t i = 0; i < lines[l].size(); i++) {
            rlColor4ub(lines[l][i].color.r, lines[l][i].color.g, lines[l][i].color.b, lines[l][i].color.a);
            rlVertex2f(lines[l][i].start.x, lines[l][i].start.y);
            rlVertex2f(lines[l][i].end.x, lines[l][i].end.y);
        }
    }
    rlEnd();
}
