#pragma once
#ifndef GAME_STATE_CONTROLLER_H
#define GAME_STATE_CONTROLLER_H

#include <memory>
#include "game/game.h"

struct CurrentGameInfo;

struct GameStateTemplate {
    const int screenWidth = 720;
    const int screenHeight = 720;
    virtual ~GameStateTemplate() = default;
    virtual void OnLoad(CurrentGameInfo& info) = 0;
    virtual void OnUnload(CurrentGameInfo& info) = 0;

    virtual void OnUpdate(CurrentGameInfo& info) = 0;
    virtual void OnRender(CurrentGameInfo& info) = 0;
};

struct MainMenuState : GameStateTemplate {
    void OnLoad(CurrentGameInfo& info) override;
    void OnUnload(CurrentGameInfo& info) override;
    void OnUpdate(CurrentGameInfo& info) override;
    void OnRender(CurrentGameInfo& info) override;
};

struct InGameState : GameStateTemplate {
    Game game{};
    
    void OnLoad(CurrentGameInfo& info) override;
    void OnUnload(CurrentGameInfo& info) override;
    void OnUpdate(CurrentGameInfo& info) override;
    void OnRender(CurrentGameInfo& info) override;
};

struct LevelEditorState : GameStateTemplate {
    Game game{};

    void OnLoad(CurrentGameInfo& info) override;
    void OnUnload(CurrentGameInfo& info) override;
    void OnUpdate(CurrentGameInfo& info) override;
    void OnRender(CurrentGameInfo& info) override;
};

struct GameStateController {
    std::shared_ptr<GameStateTemplate> gameState = nullptr;

    GameStateController(CurrentGameInfo& info);

    void LoadGameState(CurrentGameInfo& info, std::shared_ptr<GameStateTemplate> newState);

    void OnUpdate(CurrentGameInfo& info);

    void OnRender(CurrentGameInfo& info);

    void Unload(CurrentGameInfo& info);
};
#endif