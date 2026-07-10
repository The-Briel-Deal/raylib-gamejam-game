#pragma once
#ifndef GAME_STATE_CONTROLLER_H
#define GAME_STATE_CONTROLLER_H

#include <memory>
#include "game/game.h"
#include "gui_common.h"
#include <string>

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
    GuiFrame menu_frame{};
    Game game{};
    
    void OnLoad(CurrentGameInfo& info) override;
    void OnUnload(CurrentGameInfo& info) override;
    void OnUpdate(CurrentGameInfo& info) override;
    void OnRender(CurrentGameInfo& info) override;
};

enum class EditingMode {
    RAISE, LOWER, PAINT, SMOOTH, SET_HEIGHT
};

struct LevelEditorState : GameStateTemplate {
    Game game{};
    GuiFrame edit_frame{};
    EditingMode editMode = EditingMode::RAISE;
    int setHeight = 1;
    int radius = 1;
    int power = 100;
    int rgb_paint = 0xffffff;
    Color paintColor;
    bool load_screen = false;
    std::string name = "Level Name";

    void OnLoad(CurrentGameInfo& info) override;
    void OnUnload(CurrentGameInfo& info) override;
    void OnUpdate(CurrentGameInfo& info) override;
    void OnRender(CurrentGameInfo& info) override;
};

struct GameStateController {
    std::shared_ptr<GameStateTemplate> gameState = nullptr;

    GuiFrame frame{};

    GameStateController(CurrentGameInfo& info);

    void LoadGameState(CurrentGameInfo& info, std::shared_ptr<GameStateTemplate> newState);

    void OnUpdate(CurrentGameInfo& info);

    void OnRender(CurrentGameInfo& info);

    void Unload(CurrentGameInfo& info);
};
#endif