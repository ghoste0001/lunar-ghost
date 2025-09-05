#pragma once
#include <raylib.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "bootstrap/Instance.h"

class TopBarPanel;
class ExplorerPanel;
class PropertyPanel;
class PreferencesWindow;

class GuiManager {
public: 
    GuiManager();
    ~GuiManager();

    void Initialize();
    void Update();
    void Render();
    void Shutdown();

    // Toggle functionality
    void HandleInput();
    bool IsGuiVisible() const { return guiVisible; }
    void SetGuiVisible(bool visible);

    // Selection management
    void SetSelectedInstance(std::shared_ptr<Instance> instance);
    std::shared_ptr<Instance> GetSelectedInstance() const { return selectedInstance; }

    // Mouse control
    bool IsMouseCaptured() const { return mouseCaptured; }
    void SetMouseCaptured(bool captured);

    // Font management
    static Font GetCustomFont() { return customFont; }
    static bool IsCustomFontLoaded() { return customFontLoaded; }

    // Icon management
    static Texture2D GetInstanceIcon(const std::string& className);
    static bool IsIconLoaded(const std::string& className);
    
    // Preferences window
    void ShowPreferences();
    void HidePreferences();

private:    
    bool guiVisible = false;
    bool mouseCaptured = false;

    // Topbar state
    bool fileDropdownOpen = false;
    bool editDropdownOpen = false;

    int fiveKeyPressCount = 0;
    int topbarDropdownHoveredItem = -1; // optional, for click selection

    double lastFiveKeyTime = 0.0;
    static constexpr double FIVE_KEY_TIMEOUT = 2.0; // 2 seconds timeout for 5 key sequence
    static constexpr int FIVE_KEY_REQUIRED_COUNT = 5;

    std::shared_ptr<Instance> selectedInstance;
    
    std::unique_ptr<TopBarPanel> topBarPanel;
    std::unique_ptr<ExplorerPanel> explorerPanel;
    std::unique_ptr<PropertyPanel> propertyPanel;
    std::unique_ptr<PreferencesWindow> preferencesWindow;

    // Font management
    static Font customFont;
    static bool customFontLoaded;
    void LoadCustomFont();

    // Icon management
    static std::unordered_map<std::string, Texture2D> iconCache;
    void LoadInstanceIcons();
    void UnloadInstanceIcons();

    // GUI styling
    void SetupDarkTheme();
    
    // Layout constants
    static constexpr float EXPLORER_WIDTH = 300.0f;
    static constexpr float PROPERTY_WIDTH = 350.0f;
    static constexpr float PANEL_HEIGHT_RATIO = 0.8f;
    static constexpr float PANEL_MARGIN = 10.0f;

    // Internal helpers
    void RenderTopbarDropdowns();    
};
