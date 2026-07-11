#pragma once
#include <glm/glm.hpp>
using namespace glm;
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

enum class ComponentType {
    BUTTON, LABEL, TEXTURE_BUTTON, TEXTURE, TEXT_INPUT, FLOAT_SLIDER, CHECKBOX, INT_SLIDER
};

struct GuiStyle {
    int StyleID = 0;
    Color text_color = BLACK;
    Color button_down_color = DARKGREEN;
    Color button_up_color = GREEN;
    Color background_color = GRAY;
    Color border_color = BLACK;
    Color image_tint = WHITE;
    Color image_button_hover_tint = GRAY;
    Color slider_background_color = BLACK;
    Color slider_color = RED;

    Color checkbox_color = DARKGRAY;
    Color checkbox_hover_color = GRAY;
    Color checkbox_select_color = GREEN;

    Color textbox_background_color = WHITE;
    Color textbox_background_color_active = GRAY;

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

struct CheckboxInfo : GuiInfo {
    
    bool hovered = false;
    bool& active;

    CheckboxInfo(bool& active) : active(active) {}

    void Draw(GuiStyle& style) override {
        Rectangle rect = {pos.x, pos.y, size.x, size.y};
        DrawRectangleRec(rect, active ? style.checkbox_select_color : hovered ? style.checkbox_hover_color : style.checkbox_color);
        DrawRectangleLinesEx(rect, style.border_size, style.border_color);

        if (hovered) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                active = !active;
            }
        }
    }

    ComponentType GetType() override {
        return ComponentType::CHECKBOX;
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

struct TextureButtonInfo : GuiInfo {
    Texture2D texture;
    Rectangle sourceRect{};
    bool hovered = false;

    void Draw(GuiStyle& style) override {
        if (hovered) {
            DrawTexturePro(texture, sourceRect, {pos.x, pos.y, size.x, size.y}, {}, 0, style.image_button_hover_tint);
        } else {
            DrawTexturePro(texture, sourceRect, {pos.x, pos.y, size.x, size.y}, {}, 0, style.image_tint);
        }
    }

    ComponentType GetType() override {
        return ComponentType::TEXTURE_BUTTON;
    }
};

struct TextureInfo : GuiInfo {
    Texture2D texture{};
    Rectangle sourceRect{};
    f32vec2 origin{};
    f32 rotation;

    void Draw(GuiStyle& style) override {
        DrawTexturePro(texture, sourceRect, {pos.x, pos.y, size.x, size.y}, {origin.x, origin.y}, rotation, style.image_tint);
    }

    ComponentType GetType() override {
        return ComponentType::TEXTURE;
    }
};

struct TextBoxInfo : GuiInfo {
    int max_characters;
    std::string text;
    bool active = false;
    bool hovered = false;

    void Draw(GuiStyle& style) override {
        Rectangle rect = {pos.x, pos.y, size.x, size.y};
        DrawRectangleRec(rect, active ? style.textbox_background_color_active : style.textbox_background_color);
        DrawText(text.c_str(), (int)pos.x + style.text_padding, (int)pos.y + style.text_padding, style.text_size, style.text_color);
        DrawRectangleLinesEx(rect, style.border_size, style.border_color);
    }

    ComponentType GetType() override {
        return ComponentType::TEXT_INPUT;
    }
};

struct FloatSliderInfo : GuiInfo {
    float& value;
    float min_value = 0;
    float max_value = 1;
    bool hovered = false;
    bool active = false;

    FloatSliderInfo(float& value) : value(value) {

    }

    void Draw(GuiStyle& style) override {
        Rectangle rect = {pos.x, pos.y, size.x, size.y};
        DrawRectangleRec(rect, style.slider_background_color);

        float value_mix = clamp(value - min_value, 0.0f, max_value - min_value) / abs(max_value - min_value);
        DrawRectangleRec({pos.x + (size.x - 10) * value_mix, pos.y, 10, size.y}, style.slider_color);

        auto mousePos = GetMousePosition();
        float mouse_mix = (mousePos.x - pos.x) / size.x;

        if (hovered || active) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                value = mouse_mix * (max_value - min_value) + min_value;
                value = clamp(value, min_value, max_value);
                active = true;
            } else {
                active = false;
            }
        }

    }

    ComponentType GetType() override {
        return ComponentType::FLOAT_SLIDER;
    }
};

struct IntSliderInfo : GuiInfo {
    int& value;
    int min_value = 0;
    int max_value = 1;
    bool hovered = false;
    bool active = false;

    IntSliderInfo(int& value) : value(value) {

    }

    void Draw(GuiStyle& style) override {
        Rectangle rect = {pos.x, pos.y, size.x, size.y};
        DrawRectangleRec(rect, style.slider_background_color);

        float value_mix = (float)clamp(value - min_value, 0, max_value - min_value) / (float)abs(max_value - min_value);
        DrawRectangleRec({pos.x + (size.x - 10) * value_mix, pos.y, 10, size.y}, style.slider_color);

        auto mousePos = GetMousePosition();
        float mouse_mix = (mousePos.x - pos.x) / size.x;

        if (hovered || active) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                value = (int)floor(mouse_mix * (max_value - min_value) + min_value);
                value = clamp(value, min_value, max_value);
                active = true;
            } else {
                active = false;
            }
        }

    }

    ComponentType GetType() override {
        return ComponentType::INT_SLIDER;
    }
};

struct GuiFrame {
    bool has_background = true;
    bool has_border = true;
    bool hovered = false;

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

        hovered = mousePos.x >= frame_pos.x && mousePos.y >= frame_pos.y && mousePos.x <= frame_pos.x + frame_size.x && mousePos.y <= frame_pos.y + frame_size.y;
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

    bool DrawCheckbox(std::string text, bool& active) {
        auto& style = GetCurrentStyle();
        auto text_width = MeasureText(text.c_str(), style.text_size);
        auto info = std::make_shared<CheckboxInfo>(active);
        auto label = currentLabel + std::to_string(component_count);

        info->pos = cursor_pos + frame_pos;
        info->size = {style.text_size, style.text_size};
        info->hovered = mousePos.x >= info->pos.x && mousePos.y >= info->pos.y && mousePos.x <= info->pos.x + info->size.x && mousePos.y <= info->pos.y + info->size.y;

        drawList[label] = info;
        styleID.push_back(style.StyleID);
        drawOrder.push_back(label);
        component_count++;

        last_size = info->size;

        float cx = cursor_pos.x;
        SetCursorPos({cursor_pos.x, cursor_pos.y + info->size.y + 5});
        SameLine();
        DrawLabel(text);
        SetCursorPos({cx, cursor_pos.y + info->size.y + 5});

        return info->hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    }

    bool DrawImageButton(Texture2D image, f32vec2 size = {}, Rectangle source = {}) {
        auto& style = GetCurrentStyle();
        auto info = std::make_shared<TextureButtonInfo>();
        auto label = currentLabel + std::to_string(component_count);

        info->texture = image;
        info->pos = cursor_pos + frame_pos;

        if (size.x == 0 || size.y == 0) {
            size = {image.width, image.height};
        }

        if (source.width == 0 || source.height == 0) {
            source = {0, 0, (f32)image.width, (f32)image.height};
        }

        info->size = size;
        info->sourceRect = source;
        info->hovered = mousePos.x >= info->pos.x && mousePos.y >= info->pos.y && mousePos.x <= info->pos.x + info->size.x && mousePos.y <= info->pos.y + info->size.y;

        drawList[label] = info;
        styleID.push_back(style.StyleID);
        drawOrder.push_back(label);
        component_count++;

        last_size = info->size;
        SetCursorPos({cursor_pos.x, cursor_pos.y + info->size.y + 5});

        return info->hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    }

    void DrawImage(Texture2D image, f32vec2 size = {}, Rectangle source = {}, f32vec2 origin = {}, f32 rotation = 0) {
        auto& style = GetCurrentStyle();
        auto info = std::make_shared<TextureInfo>();
        auto label = currentLabel + std::to_string(component_count);

        info->texture = image;
        info->pos = cursor_pos + frame_pos;

        if (size.x == 0 || size.y == 0) {
            size = {image.width, image.height};
        }

        if (source.width == 0 || source.height == 0) {
            source = {0, 0, (f32)image.width, (f32)image.height};
        }

        info->origin = origin;
        info->rotation = rotation;
        info->size = size;
        info->sourceRect = source;

        drawList[label] = info;
        styleID.push_back(style.StyleID);
        drawOrder.push_back(label);
        component_count++;

        last_size = info->size;
        SetCursorPos({cursor_pos.x, cursor_pos.y + info->size.y + 5});
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

    void DrawTextInput(std::string& text, int max_characters = 40) {
        auto& style = GetCurrentStyle();
        auto text_width = MeasureText("E", style.text_size) * max_characters;
        auto info = std::make_shared<TextBoxInfo>();
        auto label = currentLabel + std::to_string(component_count);

        auto find = lastDrawList.find(label);
        if (find != lastDrawList.end()) {
            if (find->second->GetType() == ComponentType::TEXT_INPUT) {
                auto last = std::dynamic_pointer_cast<TextBoxInfo>(find->second);
                info->active = last->active;
            }
        } else {
            info->active = false;
        }
        info->text = text;
        info->max_characters = max_characters;
        info->pos = cursor_pos + frame_pos;
        info->size = {text_width + style.text_padding * 2 + 20, style.text_size + style.text_padding * 2};
        info->hovered = mousePos.x >= info->pos.x && mousePos.y >= info->pos.y && mousePos.x <= info->pos.x + info->size.x && mousePos.y <= info->pos.y + info->size.y;

        drawList[label] = info;
        styleID.push_back(style.StyleID);
        drawOrder.push_back(label);
        component_count++;

        last_size = info->size;
        SetCursorPos({cursor_pos.x, cursor_pos.y + info->size.y + 5});




        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (info->hovered) {
                info->active = true;
            } else {
                info->active = false;
            }
        }
        if (info->active) {
            if (IsKeyPressed(KEY_ENTER)) {
                info->active = false;
                return;
            }
            auto key_char = GetCharPressed();
            if (IsKeyPressed(KEY_SPACE)) {
                text += " ";
            } else if (IsKeyPressed(KEY_BACKSPACE)) {
                if (text.size() > 0) {
                    text = text.substr(0, text.size() - 1);
                }
            } else if (key_char >= 32) {
                text += (char)key_char;
            }
            if (text.size() >= max_characters) {
                text = text.substr(0, static_cast<size_t>(max_characters));
            }
        }
    }

    void DrawFloatSlider(float& value, float min_value = 0, float max_value = 1, float width = 200, bool has_label = true) {
        auto& style = GetCurrentStyle();
        auto info = std::make_shared<FloatSliderInfo>(value);
        auto label = currentLabel + std::to_string(component_count);

        auto find = lastDrawList.find(label);
        if (find != lastDrawList.end()) {
            if (find->second->GetType() == ComponentType::FLOAT_SLIDER) {
                auto last = std::dynamic_pointer_cast<FloatSliderInfo>(find->second);
                info->active = last->active;
            }
        } else {
            info->active = false;
        }

        info->min_value = min_value;
        info->max_value = max_value;
        info->pos = cursor_pos + frame_pos;
        info->size = {width, style.text_size};
        info->hovered = mousePos.x >= info->pos.x && mousePos.y >= info->pos.y && mousePos.x <= info->pos.x + info->size.x && mousePos.y <= info->pos.y + info->size.y;

        drawList[label] = info;
        styleID.push_back(style.StyleID);
        drawOrder.push_back(label);
        component_count++;

        last_size = info->size;
        float cx = cursor_pos.x;

        SetCursorPos({cursor_pos.x, cursor_pos.y + info->size.y + 5});
        if (has_label) {
            SameLine();
            DrawLabel(std::to_string(value));
        }
        SetCursorPos({cx, cursor_pos.y + info->size.y + 5});
    }

    void DrawIntSlider(int& value, int min_value = 0, int max_value = 1, float width = 200, bool has_label = true) {
        auto& style = GetCurrentStyle();
        auto info = std::make_shared<IntSliderInfo>(value);
        auto label = currentLabel + std::to_string(component_count);

        auto find = lastDrawList.find(label);
        if (find != lastDrawList.end()) {
            if (find->second->GetType() == ComponentType::INT_SLIDER) {
                auto last = std::dynamic_pointer_cast<IntSliderInfo>(find->second);
                info->active = last->active;
            }
        } else {
            info->active = false;
        }

        info->min_value = min_value;
        info->max_value = max_value;
        info->pos = cursor_pos + frame_pos;
        info->size = {width, style.text_size};
        info->hovered = mousePos.x >= info->pos.x && mousePos.y >= info->pos.y && mousePos.x <= info->pos.x + info->size.x && mousePos.y <= info->pos.y + info->size.y;

        drawList[label] = info;
        styleID.push_back(style.StyleID);
        drawOrder.push_back(label);
        component_count++;

        last_size = info->size;
        float cx = cursor_pos.x;

        SetCursorPos({cursor_pos.x, cursor_pos.y + info->size.y + 5});
        if (has_label) {
            SameLine();
            DrawLabel(std::to_string(value));
        }
        SetCursorPos({cx, cursor_pos.y + info->size.y + 5});
    }
};
