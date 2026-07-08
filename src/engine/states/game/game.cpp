#include "game.h"

#include <utility>

Game::Game(Level level) : level(std::move(level)) {
    camera.x = (f32)this->level.width / 2.0f;
    camera.z = (f32)this->level.length / 2.0f;
}

void Game::BuildLevel(std::vector<LevelShape> shapes) {
    for (auto& shape : shapes) {
        
        for (int x = shape.x1; x < shape.x2; x++) {
            for (int z = shape.z1; z < shape.z2; z++) {
                if (level.InBounds(x, z)) {
                    auto& height = level.At(x, z);
                    auto& color = level.ColorAt(x, z);

                    if (shape.modifier == HeightModifier::MIN) {
                        if (height >= shape.height) {
                            height = shape.height;
                            color = shape.color;
                        }
                    } else {
                        if (height <= shape.height) {
                            height = shape.height;
                            color = shape.color;
                        }
                    }
                }
            }
        }
    }
}
