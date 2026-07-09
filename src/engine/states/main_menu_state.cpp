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
    DrawText("GAME COMPETITION ENTRY", 72, 64 + 80, 40, BLACK);

    Rectangle playButton = {64, 64 + 80 + 40, 400, 60};

    auto mousePos = GetMousePosition();

    Color playColor = GREEN;
    if (CheckCollisionPointRec(mousePos, playButton)) {
        playColor = DARKGREEN;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            info.gameStateController->LoadGameState(info, std::make_shared<InGameState>());
        }
    }

    DrawRectangleRec(playButton, playColor);
    DrawText("PLAY", playButton.x + 10, playButton.y + 10, 40, BLACK);

    Rectangle editButton = {playButton.x, playButton.y + playButton.height + 20, playButton.width, playButton.height};
    Color editColor = GREEN;
    if (CheckCollisionPointRec(mousePos, editButton)) {
        editColor = DARKGREEN;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            info.gameStateController->LoadGameState(info, std::make_shared<LevelEditorState>());
        }
    }

    DrawRectangleRec(editButton, editColor);
    DrawText("LEVEL EDIT", editButton.x + 10, editButton.y + 10, 40, BLACK);
}

void MainMenuState::OnUnload(CurrentGameInfo& info) {

}