#include "engine/rendering/render_system.h"
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

static RenderSystem* renderSystemInstance;

void MainLoop() {
    renderSystemInstance->DrawFrame();
}

int main(void) {
    auto renderSystem = RenderSystem();
    renderSystemInstance = &renderSystem;

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