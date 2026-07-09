#include "game_state_controller.h"
#include "../current_game_info.h"

GameStateController::GameStateController(CurrentGameInfo& info) {
    info.gameStateController = this;
    LoadGameState(info, std::make_shared<MainMenuState>());
}

void GameStateController::LoadGameState(CurrentGameInfo& info, std::shared_ptr<GameStateTemplate> newState) {
    Unload(info);
    newState->OnLoad(info);
    gameState = newState;
}

void GameStateController::OnUpdate(CurrentGameInfo& info) {
    if (gameState == nullptr) {
        return;
    }
    gameState->OnUpdate(info);
}

void GameStateController::OnRender(CurrentGameInfo& info) {
    if (gameState == nullptr) {
        return;
    }
    frame.has_background = false;
    frame.has_border = false;
    frame.frame_size = {720, 720};
    frame.BeginDrawing();
    gameState->OnRender(info);
    frame.EndDrawing();
}

void GameStateController::Unload(CurrentGameInfo& info) {
    if (gameState == nullptr) {
        return;
    }
    gameState->OnUnload(info);
    gameState = nullptr;
}