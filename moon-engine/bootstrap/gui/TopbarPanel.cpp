#include "TopBarPanel.h"
#include "GuiManager.h"
#include <filesystem>
namespace fs = std::filesystem;

TopBarPanel::TopBarPanel(GuiManager* manager, Font customFont)
    : guiManager(manager), font(customFont)
{
    menuItems = { "File", "Edit", "Render", "Window", "Help" };

    fileMenu.push_back({ "New Project" });
    fileMenu.push_back({ "Save As" });
    fileMenu.push_back({ "Extension", {}, true });
    LoadExtensions();
    
    // Initialize Edit menu
    editMenu.push_back({ "Preferences..." });
}

TopBarPanel::~TopBarPanel() {}

void TopBarPanel::LoadExtensions() {
    fileMenu[2].subItems.clear();
    std::string path = "resources/eclipsera-contents/extensions";

    if (fs::exists(path) && fs::is_directory(path)) {
        for (auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                fileMenu[2].subItems.push_back(entry.path().filename().string());
            }
        }
    }
}

void TopBarPanel::Update() {
    Vector2 mousePos = GetMousePosition();
    bool mouseClicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    // Detect hovered topbar item
    hoveredDropdownIndex = -1;
    float padding = 20.0f;
    float x = padding;
    int fontSize = 16;
    for (size_t i = 0; i < menuItems.size(); i++) {
        int textWidth = MeasureTextEx(font, menuItems[i].c_str(), fontSize, 1).x;
        Rectangle rect = { x, 0, (float)textWidth + padding * 2, 30 };
        if (CheckCollisionPointRec(mousePos, rect)) hoveredDropdownIndex = (int)i;
        x += rect.width;
    }

    // Handle click
    if (mouseClicked) {
        if (hoveredDropdownIndex != -1) {
            activeIndex = (activeIndex == hoveredDropdownIndex ? -1 : hoveredDropdownIndex);
            submenuVisible = false; // reset submenu
        } else {
            // Click outside: close menu
            activeIndex = -1;
            submenuVisible = false;
        }
    }

    activeSubmenuIndex = -1;
    // Keep submenu open if mouse over dropdown or submenu
    if (activeIndex == 0) { // File menu
        float width = 160;
        float itemHeight = 24;
        Vector2 mousePos = GetMousePosition();

        for (size_t i = 0; i < fileMenu.size(); i++) {
            Rectangle parentRect = {20.0f, 30.0f + i * itemHeight, width, itemHeight};

            // Check hover on parent item
            if (CheckCollisionPointRec(mousePos, parentRect) && fileMenu[i].hasSubmenu) {
                activeSubmenuIndex = (int)i;
            }

            // Check hover on submenu itself
            if (fileMenu[i].hasSubmenu && !activeSubmenuIndex) {
                Rectangle submenuRect = { parentRect.x + width - 4, parentRect.y,
                                        width, (float)fileMenu[i].subItems.size() * itemHeight };
                if (CheckCollisionPointRec(mousePos, submenuRect)) {
                    activeSubmenuIndex = (int)i;
                }
            }
        }
    } else if (activeIndex == 1) { // Edit menu
        float width = 160;
        float itemHeight = 24;
        Vector2 mousePos = GetMousePosition();

        // Calculate Edit menu position dynamically
        float editMenuX = 20.0f; // Start from File menu position
        int textWidth = MeasureTextEx(font, "File", 16, 1).x;
        editMenuX += textWidth + 40.0f; // Add File menu width + padding
        
        for (size_t i = 0; i < editMenu.size(); i++) {
            Rectangle parentRect = {editMenuX, 30.0f + i * itemHeight, width, itemHeight};

            // Handle click on Edit menu items
            if (CheckCollisionPointRec(mousePos, parentRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (editMenu[i].label == "Preferences...") {
                    // Open preferences window
                    guiManager->ShowPreferences();
                    activeIndex = -1; // Close menu
                }
            }
        }
    }
}

void TopBarPanel::Render(const Rectangle& bounds) {
    DrawRectangleRec(bounds, { 30, 30, 30, 255 });
    DrawLine(bounds.x, bounds.y + bounds.height, bounds.x + bounds.width, bounds.y + bounds.height, { 60, 60, 60, 255 });

    float padding = 20.0f;
    float x = bounds.x + padding;
    float y = bounds.y;
    float itemHeight = bounds.height;

    hoveredDropdownIndex = -1;

    for (size_t i = 0; i < menuItems.size(); i++) {
        std::string label = menuItems[i];
        int fontSize = 16;
        int textWidth = MeasureTextEx(font, label.c_str(), fontSize, 1).x;

        Rectangle itemRect = { x, y, (float)textWidth + padding * 2, itemHeight };
        bool hovered = CheckCollisionPointRec(GetMousePosition(), itemRect);
        if (hovered) hoveredDropdownIndex = (int)i;

        DrawMenuItem(label, itemRect, hovered, i == activeIndex);
        x += itemRect.width;
    }

    if (activeIndex == 0) {
        DrawDropdown(fileMenu, bounds.x + padding, bounds.y + bounds.height);
    } else if (activeIndex == 1) {
        // Calculate Edit menu position based on menu item positions
        float editMenuX = bounds.x + padding;
        for (size_t i = 0; i < 1; i++) { // Skip "File" to get to "Edit"
            int textWidth = MeasureTextEx(font, menuItems[i].c_str(), 16, 1).x;
            editMenuX += textWidth + padding * 2;
        }
        DrawDropdown(editMenu, editMenuX, bounds.y + bounds.height);
    }
}

void TopBarPanel::DrawMenuItem(const std::string& label, const Rectangle& rect, bool hovered, bool active) {
    Color bgColor = active ? Color{70, 70, 70, 255} : (hovered ? Color{50, 50, 50, 255} : Color{30, 30, 30, 255});
    Vector2 size = MeasureTextEx(font, label.c_str(), 16, 1);
    DrawRectangleRec(rect, bgColor);
    DrawTextEx(font, label.c_str(), { rect.x + (rect.width - size.x)/2, rect.y + (rect.height - size.y)/2 }, 16, 1, RAYWHITE);
}

void TopBarPanel::DrawDropdown(const std::vector<DropdownItem>& items, float x, float y) {
    float width = 160;
    float itemHeight = 24;

    for (size_t i = 0; i < items.size(); i++) {
        Rectangle rect = { x, y + i * itemHeight, width, itemHeight };
        bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
        if (hovered && items[i].hasSubmenu) submenuVisible = true;

        DrawRectangleRec(rect, hovered ? Color{60, 60, 60, 255} : Color{40, 40, 40, 255});
        DrawRectangleLinesEx(rect, 1, {20, 20, 20, 255});
        DrawTextEx(font, items[i].label.c_str(), { rect.x + 8, rect.y + 4 }, 16, 1, RAYWHITE);

        if (items[i].hasSubmenu && i == activeSubmenuIndex) {
            DrawSubmenu(items[i].subItems, rect.x + width - 4, rect.y);
        }
    }
}

void TopBarPanel::DrawSubmenu(const std::vector<std::string>& items, float x, float y) {
    float width = 160;
    float itemHeight = 24;

    for (size_t i = 0; i < items.size(); i++) {
        Rectangle rect = { x, y + i * itemHeight, width, itemHeight };
        bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
        if (hovered) hoveredSubIndex = (int)i;

        DrawRectangleRec(rect, hovered ? Color{70, 70, 70, 255} : Color{50, 50, 50, 255});
        DrawRectangleLinesEx(rect, 1, {20, 20, 20, 255});
        DrawTextEx(font, items[i].c_str(), { rect.x + 8, rect.y + 4 }, 16, 1, RAYWHITE);
    }
}