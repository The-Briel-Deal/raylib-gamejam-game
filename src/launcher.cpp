#include "engine/rendering/render_system.h"
#include "engine/states/game_state_controller.h"
#include "engine/current_game_info.h"
#include "raylib.h"


#include <flecs.h>
#include <memory>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

static CurrentGameInfo gameInfo{};
static std::unique_ptr<RenderSystem> renderSystem{};
static std::unique_ptr<GameStateController> stateController{};

void MainLoop() {
    gameInfo.frame_delta = clamp(GetFrameTime(), 1.0f / 60.0f, 1.0f);
    gameInfo.renderSystem->BeginFrame();
    gameInfo.frameCounter = gameInfo.renderSystem->frameCounter;
    gameInfo.gameStateController->OnUpdate(gameInfo);
    gameInfo.gameStateController->OnRender(gameInfo);
    gameInfo.renderSystem->EndFrame();
    gameInfo.frame_time += GetFrameTime();
}

void LoadTextures() {
    gameInfo.grungle_title_screen[0] = LoadTexture("resources/grungle_title_screen.png");
    gameInfo.grungle_title_screen[1] = LoadTexture("resources/grungle_title_screen2.png");
    gameInfo.eye_beams = LoadTexture("resources/eye_beams.png");

    gameInfo.grungle_icons[0] = LoadTexture("resources/grungle_icon_left.png");
    gameInfo.grungle_icons[1] = LoadTexture("resources/grungle_icon_middle.png");
    gameInfo.grungle_icons[2] = LoadTexture("resources/grungle_icon_right.png");
    gameInfo.grungle_icons[3] = gameInfo.grungle_icons[1];

    gameInfo.hotbar = LoadTexture("resources/grungle_hotbar.png");
}

int main(void) {
    gameInfo.flecs = std::make_unique<flecs::world>();

    renderSystem = std::make_unique<RenderSystem>();
    gameInfo.renderSystem = renderSystem.get();
    
    stateController = std::make_unique<GameStateController>(gameInfo);

    LoadTextures();
    

    #if defined(PLATFORM_WEB)
        emscripten_set_main_loop(MainLoop, 60, 1);
    #else
        SetTargetFPS(60);
        //--------------------------------------------------------------------------------------

        // Main game loop
        while (!WindowShouldClose())
        {
            MainLoop();
        }
    #endif

    stateController->Unload(gameInfo);
    renderSystem->Unload();
    gameInfo.flecs = nullptr;
}
