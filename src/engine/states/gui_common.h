#pragma once
#include <glm/glm.hpp>
using namespace glm;
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

enum class ComponentType {
    BUTTON, LABEL
};

struct GuiStyle {
    int StyleID = 0;
    Color text_color = BLACK;
    Color button_down_color = DARKGREEN;
    Color button_up_color = GREEN;
    Color background_color = GRAY;
    Color border_color = BLACK;
    i32 text_size = 20;
    i32 text_padding = 5;
    f32 border_size = 2;
};

struct GuiInfo {
    f32vec2 pos{};
    f32vec2 size{};

    virtual void Draw(GuiStyle& style) = 0;
    virtual ComponentType GetType() = 0;
};

struct ButtonInfo : GuiInfo {
    std::string text = "Button";
    bool hovered = false;

    void Draw(GuiStyle& style) override {
        Rectangle rect = {pos.x, pos.y, size.x, size.y};
        DrawRectangleRec(rect, hovered ? style.button_down_color : style.button_up_color);
        DrawText(text.c_str(), (int)pos.x + style.text_padding, (int)pos.y + style.text_padding, style.text_size, style.text_color);
        DrawRectangleLinesEx(rect, style.border_size, style.border_color);
    }

    ComponentType GetType() override {
        return ComponentType::BUTTON;
    }
};

struct LabelInfo : GuiInfo {
    std::string text = "Label";

    void Draw(GuiStyle& style) override {
        DrawText(text.c_str(), (int)pos.x + style.text_padding, (int)pos.y + style.text_padding, style.text_size, style.text_color);
    }

    ComponentType GetType() override {
        return ComponentType::LABEL;
    }
};

struct GuiFrame {
    bool has_background = true;
    bool has_border = true;

    Color background_color = DARKGRAY;
    Color border_color = BLACK;
    f32vec2 frame_pos{};
    f32vec2 cursor_pos{};
    f32vec2 frame_size{};

    std::unordered_map<std::string, std::shared_ptr<GuiInfo>> drawList;
    std::vector<GuiStyle> styleList;

    private:
    std::unordered_map<std::string, std::shared_ptr<GuiInfo>> lastDrawList;
    std::vector<std::string> drawOrder;
    std::vector<i32> styleID;
    std::vector<GuiStyle> drawStyles;
    std::vector<std::string> pushedLabel;
    std::string currentLabel = "";
    Vector2 mousePos = {};
    i32 component_count = 0;
    f32vec2 last_cursor_pos = {};
    f32vec2 last_size = {};
    public:
    void BeginDrawing() {
        cursor_pos = {};
        lastDrawList = drawList;
        drawList = {};
        styleList = {};
        styleID = {};
        styleList.push_back(GuiStyle{});
        drawStyles.push_back(GuiStyle{});
        drawOrder = {};
        pushedLabel = {};
        currentLabel = "";
        mousePos = GetMousePosition();
        component_count = 0;
        last_cursor_pos = {};
        last_size = {};
    }

    void EndDrawing() {

        Rectangle rect = {frame_pos.x, frame_pos.y, frame_size.x, frame_size.y};
        if (has_background) {
            DrawRectangleRec(rect, background_color);
        }
        if (has_border) {
            DrawRectangleLinesEx(rect, 2, border_color);
        }

        for (auto i = 0; i < drawOrder.size(); i++) {
            auto& str = drawOrder[i];
            auto component = drawList[str];
            auto ID = styleID[i];
            auto& style = drawStyles[static_cast<size_t>(ID)];

            component->Draw(style);
        }
    }

    void PushLabel(std::string label) {
        pushedLabel.push_back(label);
        currentLabel += label;
    }

    void PopLabel() {
        pushedLabel.pop_back();
        currentLabel = "";
        for (auto& str : pushedLabel) {
            currentLabel += str;
        }
    }

    void PushStyle(GuiStyle style) {
        style.StyleID = static_cast<i32>(drawStyles.size());
        drawStyles.push_back(style);
        styleList.push_back(style);
    }

    void PopStyle() {
        if (styleList.size() > 1) {
            styleList.pop_back();
        }
    }

    void SetCursorPos(f32vec2 cursor_pos) {
        last_cursor_pos = this->cursor_pos;
        this->cursor_pos = cursor_pos;
    }

    void SameLine(f32 padding = 5) {
        SetCursorPos({last_cursor_pos.x + last_size.x + padding, last_cursor_pos.y});
    }

    const GuiStyle& GetCurrentStyle() {
        return styleList.back();
    }



    bool DrawButton(std::string text) {
        auto& style = GetCurrentStyle();
        auto text_width = MeasureText(text.c_str(), style.text_size);
        auto info = std::make_shared<ButtonInfo>();
        auto label = currentLabel + std::to_string(component_count);

        info->text = text;
        info->pos = cursor_pos + frame_pos;
        info->size = {text_width + style.text_padding * 2, style.text_size + style.text_padding * 2};
        info->hovered = mousePos.x >= info->pos.x && mousePos.y >= info->pos.y && mousePos.x <= info->pos.x + info->size.x && mousePos.y <= info->pos.y + info->size.y;

        drawList[label] = info;
        styleID.push_back(style.StyleID);
        drawOrder.push_back(label);
        component_count++;

        last_size = info->size;
        SetCursorPos({cursor_pos.x, cursor_pos.y + info->size.y + 5});

        return info->hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    }

    void DrawLabel(std::string text) {
        auto& style = GetCurrentStyle();
        auto text_width = MeasureText(text.c_str(), style.text_size);
        auto info = std::make_shared<LabelInfo>();
        auto label = currentLabel + std::to_string(component_count);

        info->text = text;
        info->pos = cursor_pos + frame_pos;
        info->size = {text_width, style.text_size};

        drawList[label] = info;
        styleID.push_back(style.StyleID);
        drawOrder.push_back(label);
        component_count++;

        last_size = info->size;
        SetCursorPos({cursor_pos.x, cursor_pos.y + info->size.y + 5});
    }
};
