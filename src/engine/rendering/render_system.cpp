#include "render_system.h"

RenderSystem::RenderSystem() {
    InitWindow(screenWidth, screenHeight, "raylib gamejam template");

    target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
}

void RenderSystem::DrawFrame() {
    frameCounter++;
    BeginTextureMode(target);
        ClearBackground(RAYWHITE);
        
        DrawRectangle(70, 90, 200, 200, BLACK);
        DrawRectangle(70 + 16, 90 + 16, 200 - 32, 200 - 32, RAYWHITE);
        DrawText("raylib", 70 + 200 - MeasureText("raylib", 40) - 32, 90 + 200 - 40 - 24, 40, BLACK);

        DrawText("6.x", 290, 90 - 26, 280, BLACK);
        DrawText("GAMEJAM", 70, 90 + 210, 120, MAROON);

        if ((frameCounter/20)%2) DrawText("are you ready?", 160, 500, 50, BLACK);

        DrawRectangleLinesEx((Rectangle){ 0, 0, (float)screenWidth, (float)screenHeight }, 16.0f, BLACK);
        
    EndTextureMode();
    
    BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw render texture to screen, scaled if required
        DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, 
            (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);

    EndDrawing();
}

void RenderSystem::Unload() {
    UnloadRenderTexture(target);
    CloseWindow();
}