#pragma once

#ifndef CURRENT_GAME_INFO_H
#define CURRENT_GAME_INFO_H
#include "rendering/render_system.h"
#include "states/game_state_controller.h"
#include <flecs.h>
#include <memory>
#include <raylib.h>

struct CurrentGameInfo {
    RenderSystem* renderSystem = nullptr;
    GameStateController* gameStateController = nullptr;
    std::unique_ptr<flecs::world> flecs = nullptr;
    int frameCounter = 0;

    Texture2D grungle_title_screen[2];
    Texture2D eye_beams;
    Texture2D grungle_icons[4];
    Texture2D hotbar;
    f32 frame_delta = 0.0f;
    f32 frame_time = 0.0f;
};

#endif