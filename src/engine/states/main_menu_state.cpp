#include "game_state_controller.h"
#include <raylib.h>

void MainMenuState::OnLoad(CurrentGameInfo& info) {

}

void MainMenuState::OnUpdate(CurrentGameInfo& info) {
    
}

void MainMenuState::OnRender(CurrentGameInfo& info) {
    DrawRectangle(70, 90, 200, 200, BLACK);
    DrawRectangle(70 + 16, 90 + 16, 200 - 32, 200 - 32, RAYWHITE);
    DrawText("raylib", 70 + 200 - MeasureText("raylib", 40) - 32, 90 + 200 - 40 - 24, 40, BLACK);

    DrawText("6.x", 290, 90 - 26, 280, BLACK);
    DrawText("GAMEJAM", 70, 90 + 210, 120, MAROON);

    //if ((frameCounter/20)%2) DrawText("are you ready?", 160, 500, 50, BLACK);

    DrawRectangleLinesEx((Rectangle){ 0, 0, (float)screenWidth, (float)screenHeight }, 16.0f, BLACK);
}

void MainMenuState::OnUnload(CurrentGameInfo& info) {

}