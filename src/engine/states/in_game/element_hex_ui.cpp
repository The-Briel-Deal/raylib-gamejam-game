#include "in_game_common.h"

#include <algorithm>
#include <cmath>

using namespace glm;

namespace in_game {
namespace {
constexpr f32 HEX_RADIUS = 47.0f;
constexpr f32 HEX_SPACING = 82.0f;

Vector2 HexSlot(i32 index)
{
    f32 center = (f32)GetScreenWidth()*0.5f;
    return {center + ((f32)index - 1.0f)*HEX_SPACING, (f32)GetScreenHeight() - 66.0f};
}

bool PointInHex(Vector2 point, Vector2 center)
{
    f32 x = fabsf(point.x - center.x);
    f32 y = fabsf(point.y - center.y);
    return (x <= HEX_RADIUS*0.8660254f) && (y <= HEX_RADIUS) &&
        (0.5773503f*x + y <= HEX_RADIUS);
}

void DrawFire(Vector2 center)
{
    Vector2 outer[] = {
        {center.x, center.y - 25.0f}, {center.x + 17.0f, center.y - 2.0f},
        {center.x + 12.0f, center.y + 20.0f}, {center.x, center.y + 27.0f},
        {center.x - 15.0f, center.y + 17.0f}, {center.x - 16.0f, center.y + 2.0f},
        {center.x - 7.0f, center.y - 10.0f}, {center.x - 5.0f, center.y + 5.0f}
    };
    DrawTriangleFan(outer, 8, Color{255, 102, 35, 255});
    DrawTriangle({center.x, center.y - 3.0f}, {center.x + 8.0f, center.y + 18.0f},
        {center.x - 9.0f, center.y + 18.0f}, Color{255, 218, 72, 255});
}

void DrawIce(Vector2 center)
{
    constexpr f32 arm = 25.0f;
    for (i32 i = 0; i < 3; i++) {
        f32 angle = (f32)i*PI/3.0f;
        Vector2 dir = {cosf(angle), sinf(angle)};
        Vector2 side = {-dir.y, dir.x};
        Vector2 a = {center.x - dir.x*arm, center.y - dir.y*arm};
        Vector2 b = {center.x + dir.x*arm, center.y + dir.y*arm};
        DrawLineEx(a, b, 4.0f, Color{220, 250, 255, 255});
        for (i32 sign : {-1, 1}) {
            Vector2 tip = {center.x + dir.x*arm*(f32)sign, center.y + dir.y*arm*(f32)sign};
            Vector2 root = {center.x + dir.x*14.0f*(f32)sign, center.y + dir.y*14.0f*(f32)sign};
            DrawLineEx(tip, {root.x + side.x*7.0f, root.y + side.y*7.0f}, 3.0f, RAYWHITE);
            DrawLineEx(tip, {root.x - side.x*7.0f, root.y - side.y*7.0f}, 3.0f, RAYWHITE);
        }
    }
    DrawCircleV(center, 5.0f, RAYWHITE);
}

void DrawWind(Vector2 center)
{
    Color color = Color{230, 255, 239, 255};
    DrawLineEx({center.x - 25.0f, center.y - 13.0f}, {center.x + 10.0f, center.y - 13.0f}, 4.0f, color);
    DrawCircleLines((i32)(center.x + 10.0f), (i32)(center.y - 6.0f), 7.0f, color);
    DrawLineEx({center.x - 29.0f, center.y}, {center.x + 25.0f, center.y}, 4.0f, color);
    DrawLineEx({center.x - 21.0f, center.y + 13.0f}, {center.x + 8.0f, center.y + 13.0f}, 4.0f, color);
    DrawCircleLines((i32)(center.x + 8.0f), (i32)(center.y + 7.0f), 6.0f, color);
}

void DrawHex(ElementHex element, Vector2 center, bool lifted)
{
    Color fill = Color{48, 50, 60, 245};
    Color border = Color{233, 202, 126, 255};
    if (element == ElementHex::FIRE) fill = Color{93, 35, 30, 250};
    if (element == ElementHex::ICE) fill = Color{29, 79, 111, 250};
    if (element == ElementHex::WIND) fill = Color{38, 91, 72, 250};
    if (lifted) {
        DrawPoly({center.x + 4.0f, center.y + 6.0f}, 6, HEX_RADIUS + 3.0f, 30.0f, Color{0, 0, 0, 100});
    }
    DrawPoly(center, 6, HEX_RADIUS, 30.0f, fill);
    DrawPolyLinesEx(center, 6, HEX_RADIUS - 2.0f, 30.0f, lifted ? 5.0f : 3.0f, lifted ? WHITE : border);
    if (element == ElementHex::FIRE) DrawFire(center);
    if (element == ElementHex::ICE) DrawIce(center);
    if (element == ElementHex::WIND) DrawWind(center);
}
}

void UpdateElementHexUi(Game& game)
{
    if (game.paused) return;
    if (IsKeyPressed(KEY_TAB)) {
        game.arranging_hexes = !game.arranging_hexes;
        game.dragged_hex = -1;
        if (game.arranging_hexes) EnableCursor();
        else DisableCursor();
    }
    if (!game.arranging_hexes) return;

    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (i32 i = 2; i >= 0; i--) {
            Vector2 slot = HexSlot(i);
            if (PointInHex(mouse, slot)) {
                game.dragged_hex = i;
                game.hex_drag_offset = vec2(slot.x - mouse.x, slot.y - mouse.y);
                game.dragged_hex_position = vec2(slot.x, slot.y);
                break;
            }
        }
    }
    if (game.dragged_hex >= 0) {
        game.dragged_hex_position = vec2(mouse.x, mouse.y) + game.hex_drag_offset;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            i32 nearest = 0;
            f32 nearestDistance = 1000000.0f;
            for (i32 i = 0; i < 3; i++) {
                Vector2 slot = HexSlot(i);
                f32 dx = mouse.x - slot.x;
                f32 dy = mouse.y - slot.y;
                f32 distance = sqrtf(dx*dx + dy*dy);
                if (distance < nearestDistance) {
                    nearestDistance = distance;
                    nearest = i;
                }
            }
            std::swap(game.element_hexes[(size_t)game.dragged_hex], game.element_hexes[(size_t)nearest]);
            game.dragged_hex = -1;
        }
    }
}

void DrawElementHexUi(const Game& game)
{
    DrawRectangleGradientV(0, GetScreenHeight() - 130, GetScreenWidth(), 130,
        Color{10, 12, 18, 0}, Color{10, 12, 18, 205});
    for (i32 i = 0; i < 3; i++) {
        if (i != game.dragged_hex) DrawHex(game.element_hexes[(size_t)i], HexSlot(i), false);
    }
    if (game.dragged_hex >= 0) {
        DrawHex(game.element_hexes[(size_t)game.dragged_hex],
            {game.dragged_hex_position.x, game.dragged_hex_position.y}, true);
    }
    const char *hint = game.arranging_hexes ? "DRAG TO REARRANGE  /  TAB TO LOCK" : "TAB  ARRANGE HEXES";
    i32 size = 14;
    DrawText(hint, GetScreenWidth()/2 - MeasureText(hint, size)/2, GetScreenHeight() - 126, size,
        game.arranging_hexes ? Color{255, 236, 184, 255} : Color{210, 214, 222, 190});
}
}
