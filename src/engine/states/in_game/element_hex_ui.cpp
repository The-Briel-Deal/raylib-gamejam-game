#include "in_game_common.h"

#include <algorithm>
#include <cmath>
#include <string>

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

Color ElementColor(ElementHex element)
{
    if (element == ElementHex::FIRE) return Color{255, 91, 42, 255};
    if (element == ElementHex::ICE) return Color{100, 220, 255, 255};
    return Color{113, 245, 175, 255};
}

const char *ElementPrefix(ElementHex element)
{
    if (element == ElementHex::FIRE) return "EMBER";
    if (element == ElementHex::ICE) return "FROST";
    return "GALE";
}

const char *WeaponForm(ElementHex element)
{
    if (element == ElementHex::FIRE) return "BLADE";
    if (element == ElementHex::ICE) return "LANCE";
    return "STAFF";
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

void DrawElementWeapon(const Game& game)
{
    ElementHex core = game.element_hexes[0];
    ElementHex form = game.element_hexes[1];
    ElementHex accent = game.element_hexes[2];
    Color coreColor = ElementColor(core);
    Color accentColor = ElementColor(accent);

    f32 bob = game.arranging_hexes ? 0.0f : sinf(game.time*0.055f)*3.0f;
    Vector2 grip = {(f32)GetScreenWidth() - 65.0f, (f32)GetScreenHeight() - 122.0f + bob};
    Vector2 guard = {grip.x - 49.0f, grip.y - 72.0f};
    Vector2 tip = {guard.x - 90.0f, guard.y - 117.0f};

    // The hand and wrapped grip anchor the weapon in first-person view.
    DrawCircle((i32)(grip.x + 22.0f), (i32)(grip.y + 34.0f), 42.0f, Color{105, 66, 48, 255});
    DrawLineEx(grip, guard, 22.0f, Color{45, 31, 35, 255});
    for (i32 i = 0; i < 4; i++) {
        f32 t = (f32)i/4.0f;
        Vector2 p = {grip.x + (guard.x - grip.x)*t, grip.y + (guard.y - grip.y)*t};
        DrawLineEx({p.x - 8.0f, p.y + 4.0f}, {p.x + 8.0f, p.y - 4.0f}, 3.0f, accentColor);
    }

    if (form == ElementHex::FIRE) {
        // Broad elemental blade.
        Vector2 blade[] = {
            {guard.x - 18.0f, guard.y + 8.0f}, {tip.x, tip.y},
            {guard.x + 8.0f, guard.y - 19.0f}, {guard.x + 22.0f, guard.y - 2.0f}
        };
        DrawTriangleFan(blade, 4, ColorAlpha(coreColor, 0.82f));
        DrawLineEx({guard.x - 8.0f, guard.y - 5.0f}, tip, 5.0f, RAYWHITE);
        DrawLineEx({guard.x - 27.0f, guard.y + 17.0f}, {guard.x + 27.0f, guard.y - 20.0f}, 10.0f, accentColor);
    } else if (form == ElementHex::ICE) {
        // Long lance with a faceted crystal point.
        DrawLineEx(guard, {tip.x + 13.0f, tip.y + 17.0f}, 12.0f, Color{68, 59, 70, 255});
        DrawTriangle(tip, {tip.x + 36.0f, tip.y + 16.0f}, {tip.x + 12.0f, tip.y + 43.0f}, coreColor);
        DrawTriangle(tip, {tip.x + 12.0f, tip.y + 43.0f}, {tip.x + 13.0f, tip.y + 17.0f}, ColorAlpha(accentColor, 0.9f));
        DrawCircleV(guard, 13.0f, accentColor);
    } else {
        // Crooked staff with an elemental focus suspended in its head.
        Vector2 head = {tip.x + 22.0f, tip.y + 24.0f};
        DrawLineEx(guard, head, 16.0f, Color{71, 52, 45, 255});
        DrawLineEx(head, {head.x - 18.0f, head.y - 27.0f}, 11.0f, Color{71, 52, 45, 255});
        DrawLineEx(head, {head.x + 26.0f, head.y - 10.0f}, 11.0f, Color{71, 52, 45, 255});
        DrawCircleV({head.x + 2.0f, head.y - 9.0f}, 18.0f, ColorAlpha(coreColor, 0.35f));
        DrawPoly({head.x + 2.0f, head.y - 9.0f}, 6, 11.0f, 30.0f, coreColor);
        DrawPolyLinesEx({head.x + 2.0f, head.y - 9.0f}, 6, 15.0f, 30.0f, 3.0f, accentColor);
    }

    std::string weaponName = std::string(ElementPrefix(core)) + " " + WeaponForm(form);
    i32 textSize = 18;
    i32 textX = GetScreenWidth() - MeasureText(weaponName.c_str(), textSize) - 18;
    i32 textY = GetScreenHeight() - 190;
    DrawText(weaponName.c_str(), textX + 2, textY + 2, textSize, Color{0, 0, 0, 180});
    DrawText(weaponName.c_str(), textX, textY, textSize, accentColor);
}
}
