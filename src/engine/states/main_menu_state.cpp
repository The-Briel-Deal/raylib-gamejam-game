#include "game_state_controller.h"
#include "../current_game_info.h"
#include <raylib.h>

void MainMenuState::OnLoad(CurrentGameInfo& info) {

}

void MainMenuState::OnUpdate(CurrentGameInfo& info) {
    
}

void MainMenuState::OnRender(CurrentGameInfo& info) {
    //DrawText("GRUNGLE", 64, 64, 80, BLACK);
    //DrawText("THE GREAT AND STRONGK", 72, 64 + 80, 40, BLACK);

    auto& frame = info.gameStateController->frame;

    frame.SetCursorPos({72, 64 + 80});
    auto style = frame.GetCurrentStyle();
    style.text_size = 40;
    frame.PushStyle(style);

    frame.DrawLabel("GAME COMPETITION ENTRY");

    Rectangle playButton = {64, 64 + 80 + 40, 400, 60};

    auto mousePos = GetMousePosition();

    if (info.gameStateController->frame.DrawButton("PLAY")) {
        info.gameStateController->LoadGameState(info, std::make_shared<InGameState>());
    }

    if (info.gameStateController->frame.DrawButton("LEVEL EDIT")) {
        info.gameStateController->LoadGameState(info, std::make_shared<LevelEditorState>());
    }
}

void MainMenuState::OnUnload(CurrentGameInfo& info) {

}