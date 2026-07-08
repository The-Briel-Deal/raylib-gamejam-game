#include "render_system.h"

RenderSystem::RenderSystem() {
    InitWindow(screenWidth, screenHeight, "raylib gamejam template");

    target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
}

void RenderSystem::BeginFrame() {
    frameCounter++;
    BeginTextureMode(target);
    ClearBackground(RAYWHITE);
}

void RenderSystem::EndFrame() {
    EndTextureMode();
    
    BeginDrawing();
        ClearBackground(RAYWHITE);
        
        DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, 
            (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);
    EndDrawing();
}

void RenderSystem::Unload() {
    UnloadRenderTexture(target);
    CloseWindow();
}