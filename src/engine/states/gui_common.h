#pragma once
#include <glm/glm.hpp>
using namespace glm;
#include <raylib.h>
#include <string>

inline bool DrawTextButton(f32 x, f32 y, f32 width, f32 height, std::string text, Color color = GREEN, Color down_color = DARKGREEN, Color text_color = BLACK) {
    Rectangle rect = {x, y, width, height};
    auto mouse = GetMousePosition();
    bool clicked = false;
    if (CheckCollisionPointRec(mouse, rect)) {
        color = down_color;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            clicked = true;
        }
    }
    DrawRectangleRec(rect, color);
    DrawText(text.c_str(), (int)x + 10, (int)y + 10, (int)height - 20, text_color);
    return clicked;
}