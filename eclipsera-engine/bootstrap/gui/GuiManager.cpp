#include "GuiManager.h"
#include "TopbarPanel.h"
#include "ExplorerPanel.h"
#include "PropertyPanel.h"
#include "PreferencesWindow.h"

#include "bootstrap/Game.h"
#include <raylib.h>
#include <raymath.h>
#include "bootstrap/fonts/meslo_font_data.h"

extern std::shared_ptr<Game> g_game;

// Static font variables
Font GuiManager::customFont = {};
bool GuiManager::customFontLoaded = false;

// Static icon variables
std::unordered_map<std::string, Texture2D> GuiManager::iconCache = {};

GuiManager::GuiManager() {
}

GuiManager::~GuiManager() {
    Shutdown();
}

void GuiManager::Initialize() {
    LoadCustomFont();
    LoadInstanceIcons();
    SetupDarkTheme();
    
    topBarPanel = std::make_unique<TopBarPanel>(this, customFont);
    explorerPanel = std::make_unique<ExplorerPanel>(this);
    propertyPanel = std::make_unique<PropertyPanel>(this);
    preferencesWindow = std::make_unique<PreferencesWindow>(this);

    // Set the root instance to the game's workspace if available
    if (g_game && g_game->workspace) {
        explorerPanel->SetRootInstance(g_game->workspace);
    }
}

void GuiManager::Update() {
    HandleInput();
    
    if (guiVisible) {
        topBarPanel->Update();
        explorerPanel->Update();
        propertyPanel->Update();
        preferencesWindow->Update();
    }
}

void GuiManager::Render() {
    if (!guiVisible) return;
    
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Calculate panel bounds
    float panelHeight = screenHeight * PANEL_HEIGHT_RATIO;
    float panelY = (screenHeight - panelHeight) * 0.5f;
    
    // Explorer panel (left side)
    
    Rectangle explorerBounds = {
        PANEL_MARGIN,
        panelY,
        EXPLORER_WIDTH,
        panelHeight
    };
    
    // Property panel (right side)
    Rectangle propertyBounds = {
        screenWidth - PROPERTY_WIDTH - PANEL_MARGIN,
        panelY,
        PROPERTY_WIDTH,
        panelHeight
    };
    
    // Render panels
    explorerPanel->Render(explorerBounds);
    propertyPanel->Render(propertyBounds);
    
    // File dropdown
    if (fileDropdownOpen) {
        DrawRectangle(0, 30, 150, 90, DARKGRAY);
        DrawText("New Project", 10, 35, 20, WHITE);
        DrawText("Save As", 10, 65, 20, WHITE);
        DrawText("Extension", 10, 95, 20, WHITE);
    }

    // Draw "Leave Editor" button at bottom center
    if (guiVisible) {
        const char* buttonText = "Leave Editor";
        int buttonWidth = 120;
        int buttonHeight = 35;
        int buttonX = (screenWidth - buttonWidth) / 2;
        int buttonY = screenHeight - buttonHeight - 20;

        Rectangle buttonRect = {(float)buttonX, (float)buttonY, (float)buttonWidth, (float)buttonHeight};
        Rectangle topBarBounds = {0, 0, (float)screenWidth, 30.0f};

        topBarPanel->Render(topBarBounds);

        // Check if button is hovered or clicked
        bool isHovered = CheckCollisionPointRec(GetMousePosition(), buttonRect);
        bool isClicked = isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        
        // Draw button
        Color buttonColor = isHovered ? Color{70, 70, 70, 255} : Color{50, 50, 50, 255};
        Color borderColor = isHovered ? Color{100, 100, 100, 255} : Color{80, 80, 80, 255};
        
        DrawRectangleRec(buttonRect, buttonColor);
        DrawRectangleLinesEx(buttonRect, 2.0f, borderColor);
        
        // Draw button text
        int textWidth = customFontLoaded ? (int)MeasureTextEx(customFont, buttonText, 16, 1.2f).x : MeasureText(buttonText, 16);
        int textX = buttonX + (buttonWidth - textWidth) / 2;
        int textY = buttonY + (buttonHeight - 16) / 2;
        
        if (customFontLoaded) {
            DrawTextEx(customFont, buttonText, {(float)textX, (float)textY}, 16, 1.2f, WHITE);
        } else {
            DrawText(buttonText, textX, textY, 16, WHITE);
        }
        
        // Handle button click
        if (isClicked) {
            SetGuiVisible(false);
        }
    }
    
    // Draw GUI status indicator
    if (guiVisible && mouseCaptured) {
        if (customFontLoaded) {
            DrawTextEx(customFont, "Editor Mode - Mouse Unlocked", {10, (float)(screenHeight - 30)}, 16, 1, YELLOW);
            DrawTextEx(customFont, "Use arrow keys to navigate explorer", {10, (float)(screenHeight - 50)}, 14, 1, LIGHTGRAY);
        } else {
            DrawText("Editor Mode - Mouse Unlocked", 10, screenHeight - 30, 16, YELLOW);
            DrawText("Use arrow keys to navigate explorer", 10, screenHeight - 50, 14, LIGHTGRAY);
        }
    }

    // Render preferences window on top of everything (after all other UI elements)
    preferencesWindow->Render();
}

void GuiManager::Shutdown() {
    explorerPanel.reset();
    propertyPanel.reset();
    preferencesWindow.reset();
    
    // Unload custom font
    if (customFontLoaded) {
        UnloadFont(customFont);
        customFontLoaded = false;
    }
    
    // Unload icons
    UnloadInstanceIcons();
}

void GuiManager::ShowPreferences() {
    if (preferencesWindow) {
        preferencesWindow->SetVisible(true);
    }
}

void GuiManager::HidePreferences() {
    if (preferencesWindow) {
        preferencesWindow->SetVisible(false);
    }
}

void GuiManager::HandleInput() {
    double currentTime = GetTime();
    
    // Handle 5 key sequence
    if (IsKeyPressed(KEY_FIVE)) {
        // Reset counter if too much time has passed
        if (currentTime - lastFiveKeyTime > FIVE_KEY_TIMEOUT) {
            fiveKeyPressCount = 0;
        }
        
        fiveKeyPressCount++;
        lastFiveKeyTime = currentTime;
        
        if (!guiVisible) {
            // If GUI is not visible, need 5 presses of '5' to open
            if (fiveKeyPressCount >= FIVE_KEY_REQUIRED_COUNT) {
                SetGuiVisible(true);
                fiveKeyPressCount = 0;
            }
        }
    }
    
    // Reset counter if timeout exceeded
    if (currentTime - lastFiveKeyTime > FIVE_KEY_TIMEOUT && fiveKeyPressCount > 0) {
        fiveKeyPressCount = 0;
    }
    
    // Handle escape key to close GUI
    if (IsKeyPressed(KEY_ESCAPE) && guiVisible) {
        SetGuiVisible(false);
    }
}

void GuiManager::SetGuiVisible(bool visible) {
    if (guiVisible == visible) return;
    
    guiVisible = visible;
    SetMouseCaptured(visible);
    
    if (visible) {
        // Refresh the explorer tree when GUI becomes visible
        explorerPanel->RefreshTree();
        
        // Set root instance if not already set
        if (g_game && g_game->workspace) {
            explorerPanel->SetRootInstance(g_game->workspace);
        }
    }
}

void GuiManager::SetSelectedInstance(std::shared_ptr<Instance> instance) {
    selectedInstance = instance;
    propertyPanel->SetTargetInstance(instance);
    
    // Refresh explorer to show any changes
    explorerPanel->RefreshTree();
}

void GuiManager::SetMouseCaptured(bool captured) {
    mouseCaptured = captured;
    
    if (captured) {
        EnableCursor();
    } else {
        DisableCursor();
    }
}

void GuiManager::LoadCustomFont() {
    // Load font from embedded data instead of external file
    // Using larger base size (64px) and more characters for better quality
    int fontSize = 64;  // Higher base size for better quality when scaled down
    int fontChars = 256; // More characters for better coverage
    int* fontCodepoints = nullptr; // Use default codepoints
    
    // Load font from embedded byte array
    customFont = LoadFontFromMemory(".ttf", MESLO_FONT_DATA, MESLO_FONT_SIZE, fontSize, fontCodepoints, fontChars);
    
    if (IsFontValid(customFont)) {
        // Set texture filter to linear for smoother rendering
        SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
        
        customFontLoaded = true;
        TraceLog(LOG_INFO, "Custom font loaded successfully from embedded data (size: %d, chars: %d, bytes: %d)", 
                fontSize, fontChars, MESLO_FONT_SIZE);
    } else {
        customFontLoaded = false;
        TraceLog(LOG_WARNING, "Failed to load embedded font data, using default font");
        customFont = GetFontDefault();
    }
}

void GuiManager::LoadInstanceIcons() {
    // List of common instance types to load icons for
    std::vector<std::string> iconNames = {
        "Workspace", "Folder", "Model", "Part", "MeshPart", "Camera", 
        "Script", "LocalScript", "BasePart", "Decal", "Texture",
        "Sound", "PointLight", "SpotLight", "SurfaceLight"
    };
    
    for (const std::string& iconName : iconNames) {
        // I'M. A. FUCKING MORON AAAAAAAAAAAAAAAAAaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
        // WHY EVERY FUCKING TIME THAT I BUILD THE ENGINE IT HAVE TO BEING SO DUMB SOMETIMES IT WANT LOCAL SCRIPT DIRECTORY, SOMETIMES IT FUCKING WANT LOCAL EXEC DIRECTORY
        // FUCK YOU, I'LL GO MULTIPLE DIRECTORY, SO IT CAN STOP BEING A FUCKING DUMBASS ALREADY LOL
        std::vector<std::string> pathsToTry = {
            "eclipsera-engine/build/resources/eclipsera-contents/textures/core/instance/" + iconName + ".png",
            "build/resources/eclipsera-contents/textures/core/instance/" + iconName + ".png",
            "resources/eclipsera-contents/textures/core/instance/" + iconName + ".png",
            "../build/resources/eclipsera-contents/textures/core/instance/" + iconName + ".png"
        };
        
        bool iconLoaded = false;
        for (const std::string& iconPath : pathsToTry) {
            if (FileExists(iconPath.c_str())) {
                Texture2D texture = LoadTexture(iconPath.c_str());
                if (IsTextureValid(texture)) {
                    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
                    iconCache[iconName] = texture;
                    TraceLog(LOG_INFO, "Loaded icon: %s from %s", iconName.c_str(), iconPath.c_str());
                    iconLoaded = true;
                    break;
                } else {
                    TraceLog(LOG_WARNING, "Failed to load icon texture: %s", iconPath.c_str());
                }
            }
        }
        
        if (!iconLoaded) {
            TraceLog(LOG_WARNING, "Icon file not found for: %s", iconName.c_str());
        }
    }
    
    TraceLog(LOG_INFO, "Loaded %d instance icons", (int)iconCache.size());
}

void GuiManager::UnloadInstanceIcons() {
    for (auto& pair : iconCache) {
        if (IsTextureValid(pair.second)) {
            UnloadTexture(pair.second);
        }
    }
    iconCache.clear();
    TraceLog(LOG_INFO, "Unloaded all instance icons");
}

Texture2D GuiManager::GetInstanceIcon(const std::string& className) {
    auto it = iconCache.find(className);
    if (it != iconCache.end()) {
        return it->second;
    }
    
    if (className == "Part" || className == "MeshPart") {
        it = iconCache.find("BasePart");
        if (it != iconCache.end()) {
            return it->second;
        }
    }
    
    return {};
}

bool GuiManager::IsIconLoaded(const std::string& className) {
    auto it = iconCache.find(className);
    return it != iconCache.end() && IsTextureValid(it->second);
}

void GuiManager::SetupDarkTheme() {
    // This could be expanded to set up global GUI theme settings
    // For now, individual panels handle their own styling...
}