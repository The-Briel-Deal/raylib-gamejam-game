#include "engine/rendering/render_system.h"
#include "engine/states/game_state_controller.h"
#include "engine/current_game_info.h"


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
    gameInfo.renderSystem->BeginFrame();
    gameInfo.frameCounter = gameInfo.renderSystem->frameCounter;
    gameInfo.gameStateController->OnUpdate(gameInfo);
    gameInfo.gameStateController->OnRender(gameInfo);
    gameInfo.renderSystem->EndFrame();
}

int main(void) {
    gameInfo.flecs = std::make_unique<flecs::world>();

    renderSystem = std::make_unique<RenderSystem>();
    gameInfo.renderSystem = renderSystem.get();
    
    stateController = std::make_unique<GameStateController>(gameInfo);

    #if defined(PLATFORM_WEB)
        emscripten_set_main_loop(MainLoop, 20, 1);
    #else
        SetTargetFPS(20);
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
