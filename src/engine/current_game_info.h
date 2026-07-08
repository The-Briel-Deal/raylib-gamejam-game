#pragma once

#include "rendering/render_system.h"
#include "states/game_state_controller.h"
#include <flecs.h>
#include <memory>

struct CurrentGameInfo {
    RenderSystem* renderSystem = nullptr;
    GameStateController* gameStateController = nullptr;
    std::unique_ptr<flecs::world> flecs = nullptr;
    int frameCounter = 0;
};
