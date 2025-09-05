#pragma once
#include <raylib.h>
#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <functional>
#include "bootstrap/Instance.h"

class GuiManager;

class PropertyPanel {
public:
    PropertyPanel(GuiManager* manager);
    ~PropertyPanel();

    void Render(Rectangle bounds);
    void Update();

    // Property management
    void SetTargetInstance(std::shared_ptr<Instance> instance);
    void RefreshProperties();

private:
    GuiManager* guiManager;
    std::shared_ptr<Instance> targetInstance;
    
    // Property types
    enum class PropertyType {
        String,
        Number,
        Boolean,
        Vector3,
        Color,
        Enum
    };
    
    struct Property {
        std::string name;
        std::string displayName;
        PropertyType type;
        std::variant<std::string, float, bool, ::Vector3, ::Color> value;
        std::variant<std::string, float, bool, ::Vector3, ::Color> defaultValue;
        bool readOnly = false;
        std::vector<std::string> enumOptions; // For enum properties
        
        // Callbacks for getting/setting values
        std::function<void()> getter;
        std::function<void()> setter;
    };
    
    std::vector<Property> properties;
    int selectedPropertyIndex = -1;
    
    // Rendering
    void RenderProperty(Property& prop, Rectangle& bounds, float& yOffset);
    void RenderStringProperty(Property& prop, Rectangle bounds);
    void RenderNumberProperty(Property& prop, Rectangle bounds);
    void RenderBooleanProperty(Property& prop, Rectangle bounds);
    void RenderVector3Property(Property& prop, Rectangle bounds);
    void RenderColorProperty(Property& prop, Rectangle bounds);
    void RenderEnumProperty(Property& prop, Rectangle bounds);
    
    // Property building
    void BuildPropertiesForInstance(std::shared_ptr<Instance> instance);
    void AddBasicInstanceProperties(std::shared_ptr<Instance> instance);
    void AddSpecificProperties(std::shared_ptr<Instance> instance);
    
    // Input handling
    bool HandlePropertyInput(Property& prop, Rectangle bounds);
    void ApplyPropertyChange(Property& prop);
    
    // Text input state
    struct TextInputState {
        bool active = false;
        std::string buffer;
        int cursorPos = 0;
        double blinkTimer = 0.0;
        int propertyIndex = -1;
        std::string fieldName; // For Vector3 components (x, y, z)
    };
    TextInputState textInput;
    
    // Dropdown state
    struct DropdownState {
        bool active = false;
        int propertyIndex = -1;
        int selectedIndex = -1;
        Rectangle bounds = {0, 0, 0, 0};
    };
    DropdownState dropdown;
    
    void StartTextInput(int propertyIndex, const std::string& initialValue, const std::string& fieldName = "");
    void UpdateTextInput();
    void EndTextInput(bool apply = true);
    
    // Constants
    static constexpr float PROPERTY_HEIGHT = 24.0f;
    static constexpr float LABEL_WIDTH = 120.0f;
    static constexpr float INPUT_PADDING = 4.0f;
    static constexpr float SECTION_SPACING = 8.0f;
    
    // Dark theme colors
    static constexpr Color BACKGROUND_COLOR = {35, 35, 35, 255};
    static constexpr Color PROPERTY_BG_COLOR = {45, 45, 45, 255};
    static constexpr Color PROPERTY_HOVER_COLOR = {55, 55, 55, 255};
    static constexpr Color INPUT_BG_COLOR = {25, 25, 25, 255};
    static constexpr Color INPUT_ACTIVE_COLOR = {40, 40, 40, 255};
    static constexpr Color TEXT_COLOR = {220, 220, 220, 255};
    static constexpr Color LABEL_COLOR = {180, 180, 180, 255};
    static constexpr Color BORDER_COLOR = {60, 60, 60, 255};
    static constexpr Color ACCENT_COLOR = {70, 130, 180, 255};
    
    // Scroll state
    float scrollY = 0.0f;
    float contentHeight = 0.0f;
    bool isDragging = false;
    Vector2 lastMousePos = {0, 0};
};
