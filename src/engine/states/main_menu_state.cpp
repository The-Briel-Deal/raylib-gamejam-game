#include "game_state_controller.h"
#include "../current_game_info.h"
#include <raylib.h>
#include <string>
#include "../rendering/render_funcs.h"

void MainMenuState::OnLoad(CurrentGameInfo& info) {

}

void MainMenuState::OnUpdate(CurrentGameInfo& info) {
    
}

void MainMenuState::OnRender(CurrentGameInfo& info) {
    //DrawText("GRUNGLE", 64, 64, 80, BLACK);
    //DrawText("THE GREAT AND STRONGK", 72, 64 + 80, 40, BLACK);

    DrawRectangleGradientV(0, 0, 720, 720, RED, BLACK);

    static float timer = 0.0f;
    timer += info.frame_delta;

    auto& frame = info.gameStateController->frame;

    auto grungle_index = static_cast<size_t>((int)timer & 1);

    DrawFire(timer, {300 - 50, 256 - 150, (float)info.grungle_title_screen[0].width * 3 + 100, (float)info.grungle_title_screen[0].height * 3 + 200});

    frame.SetCursorPos({300, 256});
    frame.DrawImage(info.grungle_title_screen[grungle_index], {info.grungle_title_screen[grungle_index].width * 3, info.grungle_title_screen[grungle_index].height * 3});

    frame.SetCursorPos({300 + 31 * 3, 256 + 24 * 3});
    frame.DrawImage(info.eye_beams, {info.eye_beams.width * (3 + 0.5f * sin(timer)), info.eye_beams.height * (3 + 0.5f * sin(timer))}, {}, {info.eye_beams.width * 0.5f * (3 + 0.5f * sin(timer)), info.eye_beams.height * 0.5f * (3 + 0.5f * sin(timer))}, 15 * cos(timer));

    frame.SetCursorPos({300 + 38 * 3, 256 + 24 * 3});
    frame.DrawImage(info.eye_beams, {info.eye_beams.width * (3 + 0.5f * sin(timer + 0.2f)), info.eye_beams.height * (3 + 0.5f * sin(timer + 0.2f))}, {}, {info.eye_beams.width * 0.5f * (3 + 0.5f * sin(timer + 0.2f)), info.eye_beams.height * 0.5f * (3 + 0.5f * sin(timer + 0.2f))}, 15 * cos(timer + 0.2f));


    frame.SetCursorPos({72, 64 + 80});
    auto style = frame.GetCurrentStyle();
    style.text_size = 40;
    frame.PushStyle(style);

    frame.DrawLabel("GRUNGLE");
    frame.DrawLabel("THE GREAT AND STRONGK");

    Rectangle playButton = {64, 64 + 80 + 40, 400, 60};

    auto mousePos = GetMousePosition();

    if (frame.DrawButton("PLAY")) {
        info.gameStateController->LoadGameState(info, std::make_shared<InGameState>());
    }

    if (frame.DrawButton("LEVEL EDIT")) {
        info.gameStateController->LoadGameState(info, std::make_shared<LevelEditorState>());
    }

    
}

void MainMenuState::OnUnload(CurrentGameInfo& info) {

}