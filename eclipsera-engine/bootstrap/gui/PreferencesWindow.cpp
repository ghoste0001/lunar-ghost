#include "PreferencesWindow.h"
#include "GuiManager.h"
#include <raylib.h>
#include <raymath.h>

PreferencesWindow::PreferencesWindow(GuiManager* manager) : guiManager(manager) {
    categories = {"Engine", "Studio"};
    
    // Center the window on screen
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    windowRect = {
        (screenWidth - WINDOW_WIDTH) / 2,
        (screenHeight - WINDOW_HEIGHT) / 2,
        WINDOW_WIDTH,
        WINDOW_HEIGHT
    };
}

PreferencesWindow::~PreferencesWindow() {
}

void PreferencesWindow::Update() {
    if (!visible) return;
    
    Vector2 mousePos = GetMousePosition();
    
    // Handle clicking outside window to close
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!CheckCollisionPointRec(mousePos, windowRect)) {
            visible = false;
        }
    }
    
    // Handle Escape key to close
    if (IsKeyPressed(KEY_ESCAPE)) {
        visible = false;
    }
    
    // Handle sidebar category selection
    if (CheckCollisionPointRec(mousePos, windowRect)) {
        Rectangle sidebarBounds = {
            windowRect.x + PADDING,
            windowRect.y + 30 + PADDING, // Account for title bar
            SIDEBAR_WIDTH,
            windowRect.height - 30 - PADDING * 2
        };
        
        if (CheckCollisionPointRec(mousePos, sidebarBounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            float relativeY = mousePos.y - sidebarBounds.y;
            int categoryIndex = (int)(relativeY / ITEM_HEIGHT);
            if (categoryIndex >= 0 && categoryIndex < (int)categories.size()) {
                selectedCategory = categoryIndex;
            }
        }
    }
}

void PreferencesWindow::Render() {
    if (!visible) return;
    
    DrawWindow();
}

void PreferencesWindow::DrawWindow() {
    // Draw window shadow
    Rectangle shadowRect = {windowRect.x + 4, windowRect.y + 4, windowRect.width, windowRect.height};
    DrawRectangleRec(shadowRect, {0, 0, 0, 100});
    
    // Draw window background
    DrawRectangleRec(windowRect, WINDOW_BG);
    DrawRectangleLinesEx(windowRect, 1.0f, BORDER_COLOR);
    
    // Draw title bar
    Rectangle titleBar = {windowRect.x, windowRect.y, windowRect.width, 30};
    DrawRectangleRec(titleBar, {40, 40, 40, 255});
    DrawRectangleLinesEx(titleBar, 1.0f, BORDER_COLOR);
    
    // Draw title text
    const char* title = "Preferences";
    if (GuiManager::IsCustomFontLoaded()) {
        Vector2 titleSize = MeasureTextEx(GuiManager::GetCustomFont(), title, 16, 1.2f);
        DrawTextEx(GuiManager::GetCustomFont(), title, 
                  {windowRect.x + (windowRect.width - titleSize.x) / 2, windowRect.y + 7}, 
                  16, 1.2f, TEXT_COLOR);
    } else {
        int titleWidth = MeasureText(title, 16);
        DrawText(title, (int)(windowRect.x + (windowRect.width - titleWidth) / 2), 
                (int)(windowRect.y + 7), 16, TEXT_COLOR);
    }
    
    // Draw close button
    Rectangle closeButton = {windowRect.x + windowRect.width - 25, windowRect.y + 5, 20, 20};
    bool closeHovered = CheckCollisionPointRec(GetMousePosition(), closeButton);
    DrawRectangleRec(closeButton, closeHovered ? Color{70, 70, 70, 255} : Color{50, 50, 50, 255});
    DrawRectangleLinesEx(closeButton, 1.0f, BORDER_COLOR);
    
    const char* closeText = "X";
    if (GuiManager::IsCustomFontLoaded()) {
        Vector2 closeSize = MeasureTextEx(GuiManager::GetCustomFont(), closeText, 12, 1.0f);
        DrawTextEx(GuiManager::GetCustomFont(), closeText, 
                  {closeButton.x + (closeButton.width - closeSize.x) / 2, closeButton.y + 4}, 
                  12, 1.0f, TEXT_COLOR);
    } else {
        int closeWidth = MeasureText(closeText, 12);
        DrawText(closeText, (int)(closeButton.x + (closeButton.width - closeWidth) / 2), 
                (int)(closeButton.y + 4), 12, TEXT_COLOR);
    }
    
    // Handle close button click
    if (closeHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        visible = false;
    }
    
    // Draw sidebar
    Rectangle sidebarBounds = {
        windowRect.x + PADDING,
        windowRect.y + 30 + PADDING,
        SIDEBAR_WIDTH,
        windowRect.height - 30 - PADDING * 2
    };
    DrawSidebar(sidebarBounds);
    
    // Draw content area
    Rectangle contentBounds = {
        windowRect.x + SIDEBAR_WIDTH + PADDING * 2,
        windowRect.y + 30 + PADDING,
        windowRect.width - SIDEBAR_WIDTH - PADDING * 3,
        windowRect.height - 30 - PADDING * 2
    };
    DrawContent(contentBounds);
}

void PreferencesWindow::DrawSidebar(Rectangle bounds) {
    DrawRectangleRec(bounds, SIDEBAR_BG);
    DrawRectangleLinesEx(bounds, 1.0f, BORDER_COLOR);
    
    Vector2 mousePos = GetMousePosition();
    
    for (int i = 0; i < (int)categories.size(); i++) {
        Rectangle itemRect = {
            bounds.x + PADDING,
            bounds.y + PADDING + i * ITEM_HEIGHT,
            bounds.width - PADDING * 2,
            ITEM_HEIGHT
        };
        
        bool isSelected = (i == selectedCategory);
        bool isHovered = CheckCollisionPointRec(mousePos, itemRect);
        
        Color bgColor = isSelected ? SELECTED_COLOR : (isHovered ? HOVER_COLOR : Color{0, 0, 0, 0});
        if (bgColor.a > 0) {
            DrawRectangleRec(itemRect, bgColor);
        }
        
        // Draw category text
        const char* categoryText = categories[i].c_str();
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), categoryText, 
                      {itemRect.x + 4, itemRect.y + 4}, 14, 1.2f, TEXT_COLOR);
        } else {
            DrawText(categoryText, (int)(itemRect.x + 4), (int)(itemRect.y + 4), 14, TEXT_COLOR);
        }
    }
}

void PreferencesWindow::DrawContent(Rectangle bounds) {
    DrawRectangleRec(bounds, CONTENT_BG);
    DrawRectangleLinesEx(bounds, 1.0f, BORDER_COLOR);
    
    switch (selectedCategory) {
        case 0: // Engine
            DrawEngineSettings(bounds);
            break;
        case 1: // Studio
            DrawStudioSettings(bounds);
            break;
    }
}

void PreferencesWindow::DrawEngineSettings(Rectangle bounds) {
    float yOffset = bounds.y + PADDING;
    
    // Engine Type Dropdown
    std::vector<std::string> engineOptions = {"Lunar Engine", "Compatibility", "Eclipsera Engine"};
    Rectangle engineDropdownRect = {bounds.x + PADDING, yOffset, 200, ITEM_HEIGHT};
    int engineIndex = (int)engineType;
    if (DrawDropdown(engineDropdownRect, "Engine:", engineIndex, engineOptions)) {
        engineType = (EngineType)engineIndex;
    }
    yOffset += ITEM_HEIGHT + PADDING * 2;
    
    // Unlock Framerate Checkbox
    Rectangle framerateCheckboxRect = {bounds.x + PADDING, yOffset, 200, ITEM_HEIGHT};
    DrawCheckbox(framerateCheckboxRect, "Unlock Framerate", unlockFramerate);
}

void PreferencesWindow::DrawStudioSettings(Rectangle bounds) {
    float yOffset = bounds.y + PADDING;
    
    // Theme Preset Dropdown
    std::vector<std::string> themeOptions = {"Light", "Dim", "Dark", "Black"};
    Rectangle themeDropdownRect = {bounds.x + PADDING, yOffset, 200, ITEM_HEIGHT};
    int themeIndex = (int)themePreset;
    if (DrawDropdown(themeDropdownRect, "Theme:", themeIndex, themeOptions)) {
        themePreset = (ThemePreset)themeIndex;
        // TODO: Apply theme change
    }
}

bool PreferencesWindow::DrawDropdown(Rectangle bounds, const char* label, int& selectedIndex, const std::vector<std::string>& options) {
    bool changed = false;
    
    // Draw label
    Rectangle labelRect = {bounds.x, bounds.y, 80, bounds.height};
    DrawLabel(labelRect, label);
    
    // Draw dropdown box
    Rectangle dropdownRect = {bounds.x + 85, bounds.y, bounds.width - 85, bounds.height};
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), dropdownRect);
    
    Color bgColor = isHovered ? HOVER_COLOR : Color{45, 45, 45, 255};
    DrawRectangleRec(dropdownRect, bgColor);
    DrawRectangleLinesEx(dropdownRect, 1.0f, BORDER_COLOR);
    
    // Draw selected option text
    if (selectedIndex >= 0 && selectedIndex < (int)options.size()) {
        const char* selectedText = options[selectedIndex].c_str();
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), selectedText, 
                      {dropdownRect.x + 4, dropdownRect.y + 4}, 14, 1.2f, TEXT_COLOR);
        } else {
            DrawText(selectedText, (int)(dropdownRect.x + 4), (int)(dropdownRect.y + 4), 14, TEXT_COLOR);
        }
    }
    
    // Draw dropdown arrow
    const char* arrow = "v";
    if (GuiManager::IsCustomFontLoaded()) {
        Vector2 arrowSize = MeasureTextEx(GuiManager::GetCustomFont(), arrow, 12, 1.0f);
        DrawTextEx(GuiManager::GetCustomFont(), arrow, 
                  {dropdownRect.x + dropdownRect.width - arrowSize.x - 8, dropdownRect.y + 6}, 
                  12, 1.0f, TEXT_COLOR);
    } else {
        int arrowWidth = MeasureText(arrow, 12);
        DrawText(arrow, (int)(dropdownRect.x + dropdownRect.width - arrowWidth - 8), 
                (int)(dropdownRect.y + 6), 12, TEXT_COLOR);
    }
    
    // Simple click handling - cycle through options
    if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        selectedIndex = (selectedIndex + 1) % options.size();
        changed = true;
    }
    
    return changed;
}

bool PreferencesWindow::DrawCheckbox(Rectangle bounds, const char* label, bool& checked) {
    bool changed = false;
    
    // Draw checkbox box
    Rectangle checkboxRect = {bounds.x, bounds.y + 2, 16, 16};
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), checkboxRect);
    
    Color bgColor = isHovered ? HOVER_COLOR : Color{45, 45, 45, 255};
    DrawRectangleRec(checkboxRect, bgColor);
    DrawRectangleLinesEx(checkboxRect, 1.0f, BORDER_COLOR);
    
    // Draw checkmark if checked
    if (checked) {
        DrawRectangleRec({checkboxRect.x + 3, checkboxRect.y + 3, 10, 10}, SELECTED_COLOR);
    }
    
    // Handle click
    if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        checked = !checked;
        changed = true;
    }
    
    // Draw label
    Rectangle labelRect = {bounds.x + 20, bounds.y, bounds.width - 20, bounds.height};
    DrawLabel(labelRect, label);
    
    return changed;
}

void PreferencesWindow::DrawLabel(Rectangle bounds, const char* text) {
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), text, 
                  {bounds.x, bounds.y + 4}, 14, 1.2f, TEXT_COLOR);
    } else {
        DrawText(text, (int)bounds.x, (int)(bounds.y + 4), 14, TEXT_COLOR);
    }
}

