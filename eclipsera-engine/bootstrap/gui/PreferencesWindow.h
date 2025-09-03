#pragma once
#include <raylib.h>
#include <string>
#include <vector>

class GuiManager;

enum class EngineType {
    LunarEngine = 0,
    Compatibility = 1,
    EclipseraEngine = 2
};

enum class ThemePreset {
    Light = 0,
    Dim = 1,
    Dark = 2,
    Black = 3
};

class PreferencesWindow {
public:
    PreferencesWindow(GuiManager* manager);
    ~PreferencesWindow();

    void Update();
    void Render();
    
    bool IsVisible() const { return visible; }
    void SetVisible(bool vis) { visible = vis; }

private:
    GuiManager* guiManager;
    bool visible = false;
    
    // Window properties
    Rectangle windowRect;
    static constexpr float WINDOW_WIDTH = 600.0f;
    static constexpr float WINDOW_HEIGHT = 400.0f;
    
    // Layout properties
    static constexpr float SIDEBAR_WIDTH = 150.0f;
    static constexpr float ITEM_HEIGHT = 24.0f;
    static constexpr float PADDING = 8.0f;
    
    // Categories
    std::vector<std::string> categories;
    int selectedCategory = 0;
    
    // Settings
    EngineType engineType = EngineType::EclipseraEngine;
    bool unlockFramerate = false;
    ThemePreset themePreset = ThemePreset::Dark;
    
    // UI Methods
    void DrawWindow();
    void DrawSidebar(Rectangle bounds);
    void DrawContent(Rectangle bounds);
    void DrawEngineSettings(Rectangle bounds);
    void DrawStudioSettings(Rectangle bounds);
    
    // UI Controls
    bool DrawDropdown(Rectangle bounds, const char* label, int& selectedIndex, const std::vector<std::string>& options);
    bool DrawCheckbox(Rectangle bounds, const char* label, bool& checked);
    void DrawLabel(Rectangle bounds, const char* text);
    
    // Theme colors
    static constexpr Color WINDOW_BG = {25, 25, 25, 255};
    static constexpr Color SIDEBAR_BG = {35, 35, 35, 255};
    static constexpr Color CONTENT_BG = {30, 30, 30, 255};
    static constexpr Color BORDER_COLOR = {60, 60, 60, 255};
    static constexpr Color TEXT_COLOR = {220, 220, 220, 255};
    static constexpr Color SELECTED_COLOR = {70, 130, 180, 255};
    static constexpr Color HOVER_COLOR = {50, 50, 50, 255};
};

