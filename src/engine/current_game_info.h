#pragma once

#ifndef CURRENT_GAME_INFO_H
#define CURRENT_GAME_INFO_H
#include "rendering/render_system.h"
#include "states/game_state_controller.h"
#include <flecs.h>
#include <memory>
#include <raylib.h>

struct SkeletonInfo {
    Texture2D hold;
    Texture2D hold_running[6];
    Texture2D idle;
    Texture2D look[2];
    Texture2D pistol;
    Texture2D pistol_pull[17];
    Texture2D running[6];
    Texture2D shock;
    Texture2D shoot[5];
    Texture2D sit;
    void Load() {
        hold = LoadTexture("resources/skeleton_hold.png");

        for (int i = 0; i < 6; i++) {
            std::string path = "resources/skeleton_hold_running" + std::to_string(i + 1) + ".png";
            hold_running[i] = LoadTexture(path.c_str());
        }

        idle = LoadTexture("resources/skeleton_idle.png");

        for (int i = 0; i < 2; i++) {
            std::string path = "resources/skeleton_look" + std::to_string(i + 1) + ".png";
            look[i] = LoadTexture(path.c_str());
        }

        pistol = LoadTexture("resources/skeleton_pistol.png");

        for (int i = 0; i < 17; i++) {
            std::string path = "resources/skeleton_pistol_pull" + std::to_string(i + 1) + ".png";
            pistol_pull[i] = LoadTexture(path.c_str());
        }

        for (int i = 0; i < 6; i++) {
            std::string path = "resources/skeleton_running" + std::to_string(i + 1) + ".png";
            running[i] = LoadTexture(path.c_str());
        }

        shock = LoadTexture("resources/skeleton_shock.png");

        for (int i = 0; i < 5; i++) {
            std::string path = "resources/skeleton_shoot" + std::to_string(i + 1) + ".png";
            shoot[i] = LoadTexture(path.c_str());
        }

        sit = LoadTexture("resources/skeleton_sit.png");
    }
};

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

    SkeletonInfo skeletonInfo;
};

#endif