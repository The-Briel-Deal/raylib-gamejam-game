#include "engine/rendering/render_system.h"
#include "engine/states/game_state_controller.h"
#include "engine/current_game_info.h"


#include <flecs.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

static CurrentGameInfo* gameInfo{};

void MainLoop() {
    gameInfo->renderSystem->BeginFrame();
    gameInfo->frameCounter = gameInfo->renderSystem->frameCounter;
    gameInfo->gameStateController->OnUpdate(*gameInfo);
    gameInfo->gameStateController->OnRender(*gameInfo);
    gameInfo->renderSystem->EndFrame();
}

int main(void) {

    CurrentGameInfo info{};
    auto renderSystem = RenderSystem();
    info.renderSystem = &renderSystem;
    
    auto stateController = GameStateController(info);

    gameInfo = &info;

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

    renderSystem.Unload();
}