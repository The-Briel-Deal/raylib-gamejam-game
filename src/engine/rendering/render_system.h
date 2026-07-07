#ifndef RAYLIB_GAME_H
#define RAYLIB_GAME_H

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct RenderSystem {
    const int screenWidth = 720;
    const int screenHeight = 720;

    RenderTexture2D target = { 0 };  // Render texture to render our game
    int frameCounter = 0;

    RenderSystem();
    void DrawFrame();

    void Unload();
};

#endif