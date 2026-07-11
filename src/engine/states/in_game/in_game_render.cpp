#include "in_game_common.h"
#include "../game_state_controller.h"
#include "../../current_game_info.h"

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
    float half_width = glm::tan(fov_rad * 0.5f);
    float projection_scale = (screen_h * 0.5f) / glm::tan(glm::radians(90.0f) * 0.5f);

    float yaw = glm::radians(game.yaw);
    vec2 forward_world(glm::sin(yaw), glm::cos(yaw));
    vec2 right_world(forward_world.y, -forward_world.x);

    vec2 cam_world = HexAxialToWorld(game.camera.x, game.camera.z);
    vec2 ent_world = HexAxialToWorld(entity.pos.x, entity.pos.z);
    vec2 rel = ent_world - cam_world;

    float depth = dot(rel, forward_world);
    if (depth <= 0.01f) return false;

    float side = dot(rel, right_world);
    float camera_x = side / depth;

    if (glm::abs(camera_x) > half_width) return false;

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
    const f32 half_width = glm::tan(fov_rad * 0.5f);

    f32 yaw = glm::radians(game.yaw);

    f32 forward_x = glm::sin(yaw);
    f32 forward_z = glm::cos(yaw);

    f32 right_x = glm::cos(yaw);
    f32 right_z = -glm::sin(yaw);

    const i32 screen_w = 720;
    const i32 screen_h_i = 720;
    const f32 screen_w_f = 720.0f;
    const f32 screen_h = 720.0f;
    const f32 screen_center_y = 360.0f + game.pitch * 10.0f;
    const f32 vertical_fov = glm::radians(90.0f);
    const f32 projection_scale = (screen_h * 0.5f) / glm::tan(vertical_fov * 0.5f);

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

    game.OnRender(info);

    info.flecs->each([&](VisibleEntityComponent& visible, EntityComponent& entity) {
        if (visible.visible) {
            int nausea_offset = 0;
            if (use_nausea) {
                //nausea_offset = (int)(game.nausea * glm::sin((float)i * 0.01f + nausea_time) * 15.0f);
            }

            auto ProjectY = [&](f32 world_y, f32 d) -> int {
                f32 safe_d = glm::max(d, 0.001f);
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

    DrawElementHexUi(game);

    if (game.paused) {
        auto mousePos = GetMousePosition();


        Rectangle pause_screen = PauseScreenRect(screen_w_f, screen_h);
        DrawRectangle(0, 0, screen_w, screen_h_i, Color{0, 0, 0, 120});

        menu_frame.frame_pos = {pause_screen.x, pause_screen.y};
        menu_frame.frame_size = {pause_screen.width, pause_screen.height};
        menu_frame.background_color = GRAY;

        menu_frame.BeginDrawing();

        menu_frame.SetCursorPos({95, 30});
        auto style = menu_frame.GetCurrentStyle();
        style.text_size = 48;
        menu_frame.PushStyle(style);

        menu_frame.DrawLabel("Paused");

        menu_frame.PopStyle();
        style.text_size = 40;
        menu_frame.PushStyle(style);

        menu_frame.SetCursorPos({40, 120});
        if (menu_frame.DrawButton("Continue")) {
            StartUnpause(game);
        }

        if (menu_frame.DrawButton("Exit")) {
            info.gameStateController->LoadGameState(info, std::make_shared<MainMenuState>());
        }

        menu_frame.EndDrawing();
    }

    DrawFPS(10, 10);
}
