#include "../game_state_controller.h"
#include "../in_game/in_game_common.h"
#include <raylib.h>
#include <glm/glm.hpp>
#include "../gui_common.h"
#include "../../current_game_info.h"

using namespace glm;

void LevelEditorState::OnLoad(CurrentGameInfo& info) {
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
}
void LevelEditorState::OnUnload(CurrentGameInfo& info) {

}

void LevelEditorState::OnUpdate(CurrentGameInfo& info) {

    auto& frame = info.gameStateController->frame;

    const f32 fov = 90.0f;
    const f32 fov_rad = radians(fov);
    const f32 half_width = glm::tan(fov_rad * 0.5f);

    f32 yaw = glm::radians(game.yaw);

    f32 forward_x = glm::sin(yaw);
    f32 forward_z = glm::cos(yaw);

    f32 right_x = glm::cos(yaw);
    f32 right_z = -glm::sin(yaw);

    f32vec3 walk_dir{};

    if (IsKeyDown(KEY_W)) {
        walk_dir.x += forward_x;
        walk_dir.z += forward_z;
    }
    if (IsKeyDown(KEY_S)) {
        walk_dir.x -= forward_x;
        walk_dir.z -= forward_z;
    }

    if (IsKeyDown(KEY_D)) {
        walk_dir.x += right_x;
        walk_dir.z += right_z;
    }
    if (IsKeyDown(KEY_A)) {
        walk_dir.x -= right_x;
        walk_dir.z -= right_z;
    }
    if (IsKeyDown(KEY_E)) {
        game.camera.y++;
    }
    if (IsKeyDown(KEY_Q)) {
        game.camera.y--;
    }

    auto axial_walk = in_game::HexWorldVectorToAxial({walk_dir.x, walk_dir.z});

    game.camera.x += axial_walk.x;
    game.camera.z += axial_walk.y;

    f32 sensitivity = 5;
    if (IsKeyDown(KEY_UP)) {
        game.pitch += sensitivity;
    }
    if (IsKeyDown(KEY_DOWN)) {
        game.pitch -= sensitivity;
    }
    if (IsKeyDown(KEY_LEFT)) {
        game.yaw -= sensitivity;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        game.yaw += sensitivity;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (game.select_x != -1 && game.select_z != -1) {
            game.level.ColorAt(game.select_x, game.select_z) = 0xffcc00;
        }
    }

    frame.SetCursorPos({10, 10});
    if (frame.DrawButton("Back")) {
        info.gameStateController->LoadGameState(info, std::make_shared<MainMenuState>());
    }

    game.pitch = clamp(game.pitch, -90.0f, 90.0f);
}
void LevelEditorState::OnRender(CurrentGameInfo& info) {

    game.OnRender(info);
}
