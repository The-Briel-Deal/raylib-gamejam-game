#include "../game_state_controller.h"
#include "../in_game/in_game_common.h"
#include <ios>
#include <raylib.h>
#include <glm/glm.hpp>
#include <stdexcept>
#include <string>
#include "../gui_common.h"
#include "../../current_game_info.h"
#include <filesystem>
#include <fstream>
#include <ostream>

using namespace glm;

void LevelEditorState::OnLoad(CurrentGameInfo& info) {
    game = Game(Level(1024, 1024));
    game.camera.y = 5.0f;
    game.recolor_selected = true;

    edit_frame.has_background = true;
    edit_frame.has_border = true;
    edit_frame.frame_pos = {5, 5};
    edit_frame.frame_size = {710, 150};
}
void LevelEditorState::OnUnload(CurrentGameInfo& info) {

}

void LevelEditorState::OnUpdate(CurrentGameInfo& info) {

    const f32 fov = 90.0f;
    const f32 fov_rad = radians(fov);
    const f32 half_width = glm::tan(fov_rad * 0.5f);

    f32 yaw = glm::radians(game.yaw);

    f32 forward_x = glm::sin(yaw);
    f32 forward_z = glm::cos(yaw);

    f32 right_x = glm::cos(yaw);
    f32 right_z = -glm::sin(yaw);

    f32vec3 walk_dir{};

    

    if (!edit_frame.hovered) {

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
        if (IsKeyDown(KEY_SPACE)) {
            game.camera.y++;
        }
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
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
        game.pitch = clamp(game.pitch, -90.0f, 90.0f);

        bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
        
        if (pressed) {
            
            if (game.select_x != -1 && game.select_z != -1) {
                for (int x = -radius; x <= radius; x++) {
                    for (int z = -radius; z <= radius; z++) {
                        
                        if (x * x + z * z >= radius * radius) {
                            continue;
                        }
                        switch (editMode) {
                            case EditingMode::RAISE:
                                game.level.At(game.select_x + x, game.select_z + z) += setHeight;
                                break;
                            case EditingMode::LOWER:
                                game.level.At(game.select_x + x, game.select_z + z) -= setHeight;
                                break;
                            case EditingMode::PAINT:
                                game.level.ColorAt(game.select_x + x, game.select_z + z) = rgb_paint;
                                break;

                            case EditingMode::SMOOTH:
                                {
                                    float avg = 0.0f;
                                    for (int i = -1; i <= 1; i++) {
                                        for (int j = -1; j <= 1; j++) {
                                            avg += game.level.At(game.select_x + x + i, game.select_z + z + i);
                                        }
                                    }
                                    avg /= 9.0f;
                                    float mix_val = (float)power / 100.0f;
                                    auto& at = game.level.At(game.select_x + x, game.select_z + z);
                                    at = mix(at, avg, mix_val);
                                }
                                break;
                            case EditingMode::SET_HEIGHT:
                                game.level.At(game.select_x + x, game.select_z + z) = setHeight;
                                break;
                        }
                    }
                }
                
            }
        }
        
    }

}
void LevelEditorState::OnRender(CurrentGameInfo& info) {

    game.OnRender(info);

    if (load_screen) {
        edit_frame.frame_pos = {0, 0};
        edit_frame.frame_size = {720, 720};

        edit_frame.BeginDrawing();
        if (edit_frame.DrawButton("Back to Editor")) {
            load_screen = false;
        }

        edit_frame.SetCursorPos({25, edit_frame.cursor_pos.y});

        if (!std::filesystem::exists("./levels/")) {
            std::filesystem::create_directory("./levels/");
        }

        auto it = std::filesystem::directory_iterator("./levels");
        for (auto dir : it) {
            if (dir.is_directory()) {
                continue;
            }
            if (edit_frame.DrawButton(dir.path().filename().string())) {
                std::ifstream stream(dir.path(), std::ios::binary);

                if (!stream) {
                    TraceLog(LOG_ERROR, "Could not open level file: %s",
                            dir.path().string().c_str());
                } else {
                    const std::streamsize heightsBytes =
                        static_cast<std::streamsize>(
                            sizeof(game.level.heights[0]) *
                            game.level.heights.size()
                        );

                    const std::streamsize colorsBytes =
                        static_cast<std::streamsize>(
                            sizeof(game.level.colors[0]) *
                            game.level.colors.size()
                        );

                    stream.read(
                        reinterpret_cast<char*>(game.level.heights.data()),
                        heightsBytes
                    );

                    stream.read(
                        reinterpret_cast<char*>(game.level.colors.data()),
                        colorsBytes
                    );

                    if (!stream) {
                        TraceLog(
                            LOG_ERROR,
                            "Level file is missing data or has an incompatible format"
                        );
                    } else {
                        load_screen = false;
                    }
                }
            }
        }
        edit_frame.EndDrawing();


    } else {
        edit_frame.frame_pos = {5, 5};
        edit_frame.frame_size = {710, 150};
        edit_frame.BeginDrawing();

        edit_frame.SetCursorPos({10, 10});
        if (edit_frame.DrawButton("Back")) {
            info.gameStateController->LoadGameState(info, std::make_shared<MainMenuState>());
            return;
        }

        edit_frame.SameLine();

        if (edit_frame.DrawButton("Raise")) {
            editMode = EditingMode::RAISE;
        }

        edit_frame.SameLine();

        if (edit_frame.DrawButton("Lower")) {
            editMode = EditingMode::LOWER;
        }

        edit_frame.SameLine();

        if (edit_frame.DrawButton("Paint")) {
            editMode = EditingMode::PAINT;
        }

        edit_frame.SameLine();

        if (edit_frame.DrawButton("Set Height")) {
            editMode = EditingMode::SET_HEIGHT;
        }

        edit_frame.SameLine();

        if (edit_frame.DrawButton("Smooth")) {
            editMode = EditingMode::SMOOTH;
        }

        edit_frame.SameLine();
        edit_frame.DrawTextInput(name, 15);

        edit_frame.SetCursorPos({5, edit_frame.cursor_pos.y});

        auto GetModeString = [&]() -> std::string {
            switch (editMode) {

                case EditingMode::RAISE: return "Raise";
                case EditingMode::LOWER: return "Lower";
                case EditingMode::PAINT: return "Paint";
                case EditingMode::SMOOTH: return "Smooth";
                case EditingMode::SET_HEIGHT: return "Set Height";
            }
            return "";
        };

        edit_frame.DrawLabel("Current Mode: " + GetModeString());

        edit_frame.SetCursorPos({5, edit_frame.cursor_pos.y});
        edit_frame.DrawLabel("Radius: ");
        edit_frame.SameLine();

        auto cursor_pos = edit_frame.cursor_pos;

        edit_frame.DrawIntSlider(radius, 1, 16);
        game.select_radius = radius;

        edit_frame.SameLine();
        edit_frame.SetCursorPos({edit_frame.cursor_pos.x, cursor_pos.y});
        edit_frame.DrawLabel(" | ");
        edit_frame.SameLine();
        
        if (editMode == EditingMode::SMOOTH) {
            edit_frame.DrawLabel("Power: ");
            edit_frame.SameLine();
            edit_frame.DrawIntSlider(power, 0, 100);
        } else if (editMode == EditingMode::PAINT) {
            edit_frame.DrawLabel("RGB: ");
            edit_frame.SameLine();

            int R = rgb_paint & 255;
            int G = (rgb_paint >> 8) & 255;
            int B = (rgb_paint >> 16) & 255;

            std::string rt = R == 0 ? "" : std::to_string(R);
            std::string gt = G == 0 ? "" : std::to_string(G);
            std::string bt = B == 0 ? "" : std::to_string(B);

            edit_frame.PushLabel("R");
                edit_frame.DrawTextInput(rt, 3);
            edit_frame.PopLabel();

            edit_frame.SameLine();

            edit_frame.PushLabel("G");
                edit_frame.DrawTextInput(gt, 3);
            edit_frame.PopLabel();

            edit_frame.SameLine();

            edit_frame.PushLabel("B");
                edit_frame.DrawTextInput(bt, 3);
            edit_frame.PopLabel();

            auto ParseColorChannel = [](const std::string& text) -> int {
                if (text.empty()) {
                    return 0;
                }

                try {
                    return clamp(std::stoi(text), 0, 255);
                }
                catch (const std::invalid_argument&) {
                    return 0;
                }
                catch (const std::out_of_range&) {
                    return 255;
                }
            };

            R = ParseColorChannel(rt);
            G = ParseColorChannel(gt);
            B = ParseColorChannel(bt);

            rgb_paint = (R & 255) | ((G & 255) << 8) | ((B & 255) << 16);

            auto style = edit_frame.GetCurrentStyle();
            
            style.button_up_color = {(u8)R, (u8)G, (u8)B, 255};
            style.button_down_color = {(u8)R, (u8)G, (u8)B, 255};

            edit_frame.PushStyle(style);

            edit_frame.SameLine();
            edit_frame.DrawButton("   ");

            edit_frame.PopStyle();
        } else {
            edit_frame.DrawLabel("Height: ");
            edit_frame.SameLine();
            std::string val = setHeight == 0 ? "" : std::to_string(setHeight);
            edit_frame.DrawTextInput(val, 4);
            
            if (val.empty()) {
                setHeight = 0;
            } else {
                try {
                    setHeight = std::stoi(val);
                }
                catch (const std::invalid_argument&) {
                    setHeight = 0;
                }
                catch (const std::out_of_range&) {
                    setHeight = 1;
                }
            }
        }

        edit_frame.SetCursorPos({5, edit_frame.cursor_pos.y});
        edit_frame.DrawLabel("LeftClick = single place | RightClick = continuous place");

        edit_frame.SameLine();
        edit_frame.DrawLabel(" | ");
        edit_frame.SameLine();

        if (edit_frame.DrawButton("Save")) {
            const std::filesystem::path levelsDir = "./levels";
            const std::filesystem::path savePath = levelsDir / name;

            std::error_code error;
            std::filesystem::create_directories(levelsDir, error);

            if (error) {
                TraceLog(LOG_ERROR, "Could not create levels directory: %s",
                        error.message().c_str());
            } else {
                std::ofstream stream(
                    savePath,
                    std::ios::binary | std::ios::trunc
                );

                if (!stream) {
                    TraceLog(LOG_ERROR, "Could not open save file: %s",
                            savePath.string().c_str());
                } else {
                    stream.write(
                        reinterpret_cast<const char*>(game.level.heights.data()),
                        static_cast<std::streamsize>(
                            sizeof(game.level.heights[0]) *
                            game.level.heights.size()
                        )
                    );

                    stream.write(
                        reinterpret_cast<const char*>(game.level.colors.data()),
                        static_cast<std::streamsize>(
                            sizeof(game.level.colors[0]) *
                            game.level.colors.size()
                        )
                    );

                    if (!stream) {
                        TraceLog(LOG_ERROR, "Failed while writing level data");
                    }
                }
            }
        }

        edit_frame.SameLine();

        if (edit_frame.DrawButton("Load")) {
            load_screen = true;
        }

        edit_frame.EndDrawing();
    }
}
