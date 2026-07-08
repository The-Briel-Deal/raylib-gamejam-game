#include "in_game_common.h"

using namespace glm;
using namespace in_game;

void InGameState::OnLoad(CurrentGameInfo& info) {
    /*
    struct LevelShape {
        u32 color = 0xffcc00ff;
        i32 x1, z1, x2, z2;
        HeightModifier modifier = HeightModifier::MAX;
        f32 height = 0.0f;
    };
    */
    game = Game(Level(1024, 1024));
    game.BuildLevel({
        LevelShape{
            .x1 = 512 - 25, .z1 = 512 + 25,
            .x2 = 512 + 25, .z2 = 512 + 50,
            .height = 10.0f
        },
        LevelShape{
            .color = 0xff0000,
            .x1 = 512 - 15, .z1 = 512 + 15,
            .x2 = 512 + 15, .z2 = 512 + 25,
            .height = 5.0f
        }
    });
    game.camera.y = 5.0f;

    auto playerComp = EntityComponent{};
    playerComp.pos.x = (float)game.level.width / 2.0f;
    playerComp.pos.z = (float)game.level.length / 2.0f;
    info.flecs->entity("Player").set<EntityComponent>(playerComp).set<PlayerComponent>(PlayerComponent{});

    auto testComp = EntityComponent{};
    testComp.pos = playerComp.pos;

    info.flecs->entity("Test").set<EntityComponent>(testComp).set<VisibleEntityComponent>(VisibleEntityComponent{
        .texture = LoadTexture("resources/apple.png"),
        .has_texture = true
    });
}

void InGameState::OnUnload(CurrentGameInfo& info) {

}
