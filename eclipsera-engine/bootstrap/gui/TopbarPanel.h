#pragma once
#include <raylib.h>
#include <vector>
#include <string>

class GuiManager;

struct DropdownItem {
    std::string label;
    std::vector<std::string> subItems;
    bool hasSubmenu = false;
};

class TopBarPanel {
public:
    TopBarPanel(GuiManager* manager, Font customFont);
    ~TopBarPanel();

    void Update();
    void Render(const Rectangle& bounds);

private:
    GuiManager* guiManager;
    Font font;

    std::vector<std::string> menuItems;
    int activeIndex = -1;

    std::vector<DropdownItem> fileMenu;
    std::vector<DropdownItem> editMenu;

    int hoveredDropdownIndex = -1;
    int hoveredSubIndex = -1;
    int activeSubmenuIndex = -1;
    bool submenuVisible = false;

    void LoadExtensions();
    void DrawMenuItem(const std::string& label, const Rectangle& rect, bool hovered, bool active);
    void DrawDropdown(const std::vector<DropdownItem>& items, float x, float y);
    void DrawSubmenu(const std::vector<std::string>& items, float x, float y);
};
