

#ifndef RAYLIB_GAME_H
#define RAYLIB_GAME_H

#include "raylib.h"

#include <stdio.h>

struct RenderSystem {

    RenderTexture2D target = { 0 };  // Render texture to render our game
    int frameCounter = 0;
    const int screenWidth = 720;
    const int screenHeight = 720;

    RenderSystem();

    void BeginFrame();

    void EndFrame();

    void Unload();
};

#endif