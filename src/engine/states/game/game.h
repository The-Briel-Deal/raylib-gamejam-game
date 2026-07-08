#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <raylib.h>
#include "../../../utils/math_helpers.h"

using namespace glm;

struct Level {
    u32 width;
    u32 length;

    std::vector<f32> heights;
    std::vector<u32> colors;

    inline f32& At(i32 x, i32 z) {
        return heights[static_cast<size_t>(x + z * (i32)width)];
    }

    inline u32& ColorAt(i32 x, i32 z) {
        return colors[static_cast<size_t>(x + z * (i32)width)];
    }

    inline bool InBounds(i32 x, i32 z) {
        return x >= 0 && z >= 0 && x < (int)width && z < (int)length;
    }

    inline f32 AtConst(i32 x, i32 z) {
        if (!InBounds(x, z)) {
            return 0;
        }
        return At(x, z);
    }

    inline f32vec3 NormalAt(i32 x, i32 z) {
        auto right = AtConst(x + 1, z);
        auto left = AtConst(x - 1, z);

        auto front = AtConst(x, z + 1);
        auto back = AtConst(x, z - 1);

        auto dx = right - left;
        auto dz = front - back;
        
        auto vec = f32vec3(dx, 1.0f, dz);
        return normalize(vec);
    }

    Level(u32 width = 0, u32 length = 0) : width(width), length(length) {
        heights.resize(static_cast<size_t>(width * length));
        colors.resize(static_cast<size_t>(width * length));
        for (int x = 0; x < (i32)width; x++) {
            for (int z = 0; z < (i32)length; z++) {
                auto& height = At(x, z);
                auto& color = ColorAt(x, z);

                auto fx = (float)x;
                auto fz = (float)z;
                
                height = perlin_noise(vec3(fx * 0.1f, 0, fz * 0.1f)) * 10.0f;
                color = 0x00ff00;
                
            }
        }
    }
};

enum class HeightModifier {
    MAX = 0, MIN = 1
};

struct LevelShape {
    u32 color = 0xffcc00ff;
    i32 x1, z1, x2, z2;
    HeightModifier modifier = HeightModifier::MAX;
    f32 height = 0.0f;
};

struct GameCamera {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
};

struct EntityComponent {
    f32vec3 pos{};
    f32vec3 velocity{};
    f32 radius = 3;
    f32 height = 5;
    
    bool jumping = false;
    bool on_ground = false;
    f32 walk_x = 0.0f;
    f32 walk_z = 0.0f;
    f32 step_height = 2;

    f32 walk_speed = 1.0f;
    f32 pitch = 0.0f;
    f32 yaw = 0.0f;
};

struct PlayerComponent {
    bool placeholder = false;
};

struct Game {
    GameCamera camera{};
    f32 pitch = 0.0f;
    f32 yaw = 0.0f;
    f32 gravity = -0.25f;
    f32 time = 0.0f;
    f32 nausea = 0.0f;

    bool paused = false;
    bool pause_cursor_released = false;
    i32 unpause_delay_frames = 0;
    bool pause_blocked_after_unpause = false;
    i32 cursor_hidden_after_unpause_frames = 0;
    vec2 mouse_motion{};

    Level level;

    Game(Level level = {});

    void BuildLevel(std::vector<LevelShape> shapes);

    
};
