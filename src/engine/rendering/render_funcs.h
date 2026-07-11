#pragma once

#include <raylib.h>

inline void DrawFire(float time, Rectangle rect) {
    static Shader shader = LoadShader("resources/shaders/fire.vs", "resources/shaders/fire.fs");
    static Texture2D whiteTexture = [] {
        Image image = GenImageColor(1, 1, WHITE);
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }();

    static int timeLoc = GetShaderLocation(shader, "time");

    SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);

    BeginShaderMode(shader);
    DrawTexturePro(whiteTexture, {0, 0, 1, -1}, rect, {0, 0}, 0.0f, WHITE);
    EndShaderMode();

}
