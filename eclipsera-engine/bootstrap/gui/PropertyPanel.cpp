#include "PropertyPanel.h"
#include "GuiManager.h"
#include "bootstrap/instances/BasePart.h"
#include "bootstrap/instances/Model.h"
#include "bootstrap/instances/Folder.h"
#include "bootstrap/instances/CameraGame.h"
#include "bootstrap/instances/Workspace.h"
#include <raylib.h>
#include <raymath.h>
#include <sstream>
#include <iomanip>

PropertyPanel::PropertyPanel(GuiManager* manager) : guiManager(manager) {
}

PropertyPanel::~PropertyPanel() {
}

void PropertyPanel::Render(Rectangle bounds) {
    // Draw panel background
    DrawRectangleRec(bounds, BACKGROUND_COLOR);
    DrawRectangleLinesEx(bounds, 1.0f, BORDER_COLOR);
    
    // Draw title bar
    Rectangle titleBar = {bounds.x, bounds.y, bounds.width, 25.0f};
    DrawRectangleRec(titleBar, {40, 40, 40, 255});
    
    std::string title = "Properties";
    if (targetInstance) {
        title += " - " + targetInstance->Name;
    }
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), "Properties", {titleBar.x + 8, titleBar.y + 4}, 20, 1.5f, TEXT_COLOR);
    } else {
        DrawText("Properties", (int)(titleBar.x + 8), (int)(titleBar.y + 4), 20, TEXT_COLOR);
    }
    
    // Content area
    Rectangle contentArea = {
        bounds.x + 2,
        bounds.y + 27,
        bounds.width - 4,
        bounds.height - 29
    };
    
    if (!targetInstance) {
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), "No object selected", {contentArea.x + 8, contentArea.y + 8}, 14, 1, LABEL_COLOR);
        } else {
            DrawText("No object selected", (int)(contentArea.x + 8), (int)(contentArea.y + 8), 14, LABEL_COLOR);
        }
        return;
    }
    
    // Enable scissor test for scrolling
    BeginScissorMode((int)contentArea.x, (int)contentArea.y, (int)contentArea.width, (int)contentArea.height);
    
    float yOffset = contentArea.y - scrollY;
    contentHeight = 0.0f;
    
    // Render properties
    for (size_t i = 0; i < properties.size(); i++) {
        RenderProperty(properties[i], contentArea, yOffset);
    }
    
    EndScissorMode();
    
    // Draw scrollbar if needed
    if (contentHeight > contentArea.height) {
        float scrollbarHeight = (contentArea.height / contentHeight) * contentArea.height;
        float scrollbarY = contentArea.y + (scrollY / contentHeight) * contentArea.height;
        
        Rectangle scrollbarBg = {
            contentArea.x + contentArea.width - 8,
            contentArea.y,
            8,
            contentArea.height
        };
        
        Rectangle scrollbarThumb = {
            scrollbarBg.x + 1,
            scrollbarY,
            6,
            scrollbarHeight
        };
        
        DrawRectangleRec(scrollbarBg, {20, 20, 20, 255});
        DrawRectangleRec(scrollbarThumb, {80, 80, 80, 255});
    }
    
    // Render dropdown outside scissor mode so it can extend beyond panel bounds
    // Use a semi-transparent overlay to ensure proper z-ordering
    if (dropdown.active && dropdown.propertyIndex >= 0 && dropdown.propertyIndex < (int)properties.size()) {
        Property& prop = properties[dropdown.propertyIndex];
        
        // Draw semi-transparent overlay to block interaction with other elements
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        DrawRectangle(0, 0, screenWidth, screenHeight, {0, 0, 0, 50});
        
        // Constrain dropdown bounds to screen
        Rectangle constrainedBounds = dropdown.bounds;
        if (constrainedBounds.y + constrainedBounds.height > screenHeight) {
            constrainedBounds.y = screenHeight - constrainedBounds.height - 10;
        }
        if (constrainedBounds.x + constrainedBounds.width > screenWidth) {
            constrainedBounds.x = screenWidth - constrainedBounds.width - 10;
        }
        
        // Draw dropdown background with shadow
        Rectangle shadowRect = {constrainedBounds.x + 2, constrainedBounds.y + 2, constrainedBounds.width, constrainedBounds.height};
        DrawRectangleRec(shadowRect, {0, 0, 0, 100});
        DrawRectangleRec(constrainedBounds, INPUT_BG_COLOR);
        DrawRectangleLinesEx(constrainedBounds, 2.0f, BORDER_COLOR);
        
        Vector2 mousePos = GetMousePosition();
        
        for (int i = 0; i < (int)prop.enumOptions.size(); i++) {
            Rectangle optionRect = {
                constrainedBounds.x,
                constrainedBounds.y + i * PROPERTY_HEIGHT,
                constrainedBounds.width,
                PROPERTY_HEIGHT
            };
            
            bool isHovered = CheckCollisionPointRec(mousePos, optionRect);
            bool isSelected = (i == dropdown.selectedIndex);
            
            // Draw option background
            Color optionBgColor = PROPERTY_BG_COLOR;
            if (isSelected) {
                optionBgColor = ACCENT_COLOR;
            } else if (isHovered) {
                optionBgColor = PROPERTY_HOVER_COLOR;
            }
            DrawRectangleRec(optionRect, optionBgColor);
            
            // Draw option text
            Color textColor = isSelected ? WHITE : TEXT_COLOR;
            if (GuiManager::IsCustomFontLoaded()) {
                DrawTextEx(GuiManager::GetCustomFont(), prop.enumOptions[i].c_str(), {optionRect.x + 4, optionRect.y + 2}, 18, 1.2f, textColor);
            } else {
                DrawText(prop.enumOptions[i].c_str(), (int)(optionRect.x + 4), (int)(optionRect.y + 2), 18, textColor);
            }
            
            // Handle option click
            if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                prop.value = prop.enumOptions[i];
                ApplyPropertyChange(prop);
                dropdown.active = false;
            }
        }
    }
}

void PropertyPanel::Update() {
    UpdateTextInput();
    
    // Handle scrolling
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0.0f) {
        scrollY -= wheelMove * 20.0f;
        scrollY = Clamp(scrollY, 0.0f, fmaxf(0.0f, contentHeight - 400.0f));
    }
    
    // Close dropdown if clicked outside (only when dropdown is active)
    if (dropdown.active && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();
        
        // Check if clicked on dropdown options
        Rectangle constrainedBounds = dropdown.bounds;
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        if (constrainedBounds.y + constrainedBounds.height > screenHeight) {
            constrainedBounds.y = screenHeight - constrainedBounds.height - 10;
        }
        if (constrainedBounds.x + constrainedBounds.width > screenWidth) {
            constrainedBounds.x = screenWidth - constrainedBounds.width - 10;
        }
        
        bool clickedOnDropdown = CheckCollisionPointRec(mousePos, constrainedBounds);
        
        if (!clickedOnDropdown) {
            // Close dropdown if clicked outside
            dropdown.active = false;
        }
    }
}

void PropertyPanel::SetTargetInstance(std::shared_ptr<Instance> instance) {
    if (targetInstance == instance) return;
    
    targetInstance = instance;
    RefreshProperties();
}

void PropertyPanel::RefreshProperties() {
    properties.clear();
    
    if (!targetInstance) return;
    
    BuildPropertiesForInstance(targetInstance);
}

void PropertyPanel::RenderProperty(Property& prop, Rectangle& bounds, float& yOffset) {
    float propY = yOffset;
    
    // Skip if property is outside visible area
    if (propY + PROPERTY_HEIGHT < bounds.y || propY > bounds.y + bounds.height) {
        yOffset += PROPERTY_HEIGHT;
        contentHeight += PROPERTY_HEIGHT;
        return;
    }
    
    Rectangle propRect = {
        bounds.x + INPUT_PADDING,
        propY,
        bounds.width - INPUT_PADDING * 2,
        PROPERTY_HEIGHT
    };
    
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), propRect);
    
    // Draw property background
    Color bgColor = isHovered ? PROPERTY_HOVER_COLOR : PROPERTY_BG_COLOR;
    DrawRectangleRec(propRect, bgColor);
    
    // Draw property label
    Rectangle labelRect = {
        propRect.x + INPUT_PADDING,
        propRect.y,
        LABEL_WIDTH,
        PROPERTY_HEIGHT
    };
    
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), prop.displayName.c_str(), {labelRect.x, labelRect.y + 4}, 16, 1.2f, LABEL_COLOR);
    } else {
        DrawText(prop.displayName.c_str(), (int)(labelRect.x), (int)(labelRect.y + 4), 16, LABEL_COLOR);
    }
    
    // Draw property value based on type
    Rectangle valueRect = {
        propRect.x + LABEL_WIDTH + INPUT_PADDING,
        propRect.y + 2,
        propRect.width - LABEL_WIDTH - INPUT_PADDING * 2,
        PROPERTY_HEIGHT - 4
    };
    
    switch (prop.type) {
        case PropertyType::String:
            RenderStringProperty(prop, valueRect);
            break;
        case PropertyType::Number:
            RenderNumberProperty(prop, valueRect);
            break;
        case PropertyType::Boolean:
            RenderBooleanProperty(prop, valueRect);
            break;
        case PropertyType::Vector3:
            RenderVector3Property(prop, valueRect);
            break;
        case PropertyType::Color:
            RenderColorProperty(prop, valueRect);
            break;
        case PropertyType::Enum:
            RenderEnumProperty(prop, valueRect);
            break;
    }
    
    yOffset += PROPERTY_HEIGHT;
    contentHeight += PROPERTY_HEIGHT;
}

void PropertyPanel::RenderStringProperty(Property& prop, Rectangle bounds) {
    bool isActive = textInput.active && textInput.propertyIndex == (&prop - &properties[0]);
    
    Color bgColor = isActive ? INPUT_ACTIVE_COLOR : INPUT_BG_COLOR;
    DrawRectangleRec(bounds, bgColor);
    DrawRectangleLinesEx(bounds, 1.0f, isActive ? ACCENT_COLOR : BORDER_COLOR);
    
    if (isActive) {
        // Draw text input buffer
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), textInput.buffer.c_str(), {bounds.x + 4, bounds.y + 2}, 18, 1.2f, TEXT_COLOR);
        } else {
            DrawText(textInput.buffer.c_str(), (int)(bounds.x + 4), (int)(bounds.y + 2), 18, TEXT_COLOR);
        }
        
        // Draw cursor
        if (fmod(textInput.blinkTimer, 1.0) < 0.5) {
            int textWidth;
            if (GuiManager::IsCustomFontLoaded()) {
                Vector2 textSize = MeasureTextEx(GuiManager::GetCustomFont(), textInput.buffer.substr(0, textInput.cursorPos).c_str(), 18, 1.2f);
                textWidth = (int)textSize.x;
            } else {
                textWidth = MeasureText(textInput.buffer.substr(0, textInput.cursorPos).c_str(), 18);
            }
            DrawLine((int)(bounds.x + 4 + textWidth), (int)(bounds.y + 2), 
                    (int)(bounds.x + 4 + textWidth), (int)(bounds.y + bounds.height - 2), TEXT_COLOR);
        }
    } else {
        std::string value = std::get<std::string>(prop.value);
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), value.c_str(), {bounds.x + 4, bounds.y + 2}, 18, 1.2f, TEXT_COLOR);
        } else {
            DrawText(value.c_str(), (int)(bounds.x + 4), (int)(bounds.y + 2), 18, TEXT_COLOR);
        }
    }
    
    // Handle click to start editing
    if (CheckCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !prop.readOnly) {
        std::string currentValue = std::get<std::string>(prop.value);
        StartTextInput((&prop - &properties[0]), currentValue);
    }
}

void PropertyPanel::RenderNumberProperty(Property& prop, Rectangle bounds) {
    bool isActive = textInput.active && textInput.propertyIndex == (&prop - &properties[0]);
    
    Color bgColor = isActive ? INPUT_ACTIVE_COLOR : INPUT_BG_COLOR;
    DrawRectangleRec(bounds, bgColor);
    DrawRectangleLinesEx(bounds, 1.0f, isActive ? ACCENT_COLOR : BORDER_COLOR);
    
    if (isActive) {
        // Draw text input buffer
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), textInput.buffer.c_str(), {bounds.x + 4, bounds.y + 2}, 18, 1.2f, TEXT_COLOR);
        } else {
            DrawText(textInput.buffer.c_str(), (int)(bounds.x + 4), (int)(bounds.y + 2), 18, TEXT_COLOR);
        }
        
        // Draw cursor
        if (fmod(textInput.blinkTimer, 1.0) < 0.5) {
            int textWidth;
            if (GuiManager::IsCustomFontLoaded()) {
                Vector2 textSize = MeasureTextEx(GuiManager::GetCustomFont(), textInput.buffer.substr(0, textInput.cursorPos).c_str(), 18, 1.2f);
                textWidth = (int)textSize.x;
            } else {
                textWidth = MeasureText(textInput.buffer.substr(0, textInput.cursorPos).c_str(), 18);
            }
            DrawLine((int)(bounds.x + 4 + textWidth), (int)(bounds.y + 2), 
                    (int)(bounds.x + 4 + textWidth), (int)(bounds.y + bounds.height - 2), TEXT_COLOR);
        }
    } else {
        float value = std::get<float>(prop.value);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value;
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), oss.str().c_str(), {bounds.x + 4, bounds.y + 2}, 18, 1.2f, TEXT_COLOR);
        } else {
            DrawText(oss.str().c_str(), (int)(bounds.x + 4), (int)(bounds.y + 2), 18, TEXT_COLOR);
        }
    }
    
    // Handle click to start editing
    if (CheckCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !prop.readOnly) {
        float currentValue = std::get<float>(prop.value);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << currentValue;
        StartTextInput((&prop - &properties[0]), oss.str());
    }
}

void PropertyPanel::RenderBooleanProperty(Property& prop, Rectangle bounds) {
    bool value = std::get<bool>(prop.value);
    
    // Draw checkbox
    Rectangle checkBox = {bounds.x, bounds.y + 2, 16, 16};
    DrawRectangleRec(checkBox, INPUT_BG_COLOR);
    DrawRectangleLinesEx(checkBox, 1.0f, BORDER_COLOR);
    
    if (value) {
        DrawRectangleRec({checkBox.x + 3, checkBox.y + 3, 10, 10}, ACCENT_COLOR);
    }
    
    // Handle click
    if (CheckCollisionPointRec(GetMousePosition(), checkBox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        prop.value = !value;
        ApplyPropertyChange(prop);
    }
    
    // Draw value text
    const char* text = value ? "true" : "false";
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), text, {bounds.x + 20, bounds.y + 2}, 18, 1.2f, TEXT_COLOR);
    } else {
        DrawText(text, (int)(bounds.x + 20), (int)(bounds.y + 2), 18, TEXT_COLOR);
    }
}

void PropertyPanel::RenderVector3Property(Property& prop, Rectangle bounds) {
    ::Vector3 value = std::get<::Vector3>(prop.value);
    
    float componentWidth = (bounds.width - 8) / 3.0f;
    
    // X component
    Rectangle xRect = {bounds.x, bounds.y, componentWidth, bounds.height};
    DrawRectangleRec(xRect, INPUT_BG_COLOR);
    DrawRectangleLinesEx(xRect, 1.0f, BORDER_COLOR);
    
    std::ostringstream xss;
    xss << std::fixed << std::setprecision(1) << value.x;
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), ("X:" + xss.str()).c_str(), {xRect.x + 2, xRect.y + 2}, 14, 1.2f, TEXT_COLOR);
    } else {
        DrawText(("X:" + xss.str()).c_str(), (int)(xRect.x + 2), (int)(xRect.y + 2), 14, TEXT_COLOR);
    }
    
    // Y component
    Rectangle yRect = {bounds.x + componentWidth + 2, bounds.y, componentWidth, bounds.height};
    DrawRectangleRec(yRect, INPUT_BG_COLOR);
    DrawRectangleLinesEx(yRect, 1.0f, BORDER_COLOR);
    
    std::ostringstream yss;
    yss << std::fixed << std::setprecision(1) << value.y;
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), ("Y:" + yss.str()).c_str(), {yRect.x + 2, yRect.y + 2}, 14, 1.2f, TEXT_COLOR);
    } else {
        DrawText(("Y:" + yss.str()).c_str(), (int)(yRect.x + 2), (int)(yRect.y + 2), 14, TEXT_COLOR);
    }
    
    // Z component
    Rectangle zRect = {bounds.x + componentWidth * 2 + 4, bounds.y, componentWidth, bounds.height};
    DrawRectangleRec(zRect, INPUT_BG_COLOR);
    DrawRectangleLinesEx(zRect, 1.0f, BORDER_COLOR);
    
    std::ostringstream zss;
    zss << std::fixed << std::setprecision(1) << value.z;
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), ("Z:" + zss.str()).c_str(), {zRect.x + 2, zRect.y + 2}, 14, 1.2f, TEXT_COLOR);
    } else {
        DrawText(("Z:" + zss.str()).c_str(), (int)(zRect.x + 2), (int)(zRect.y + 2), 14, TEXT_COLOR);
    }
}

void PropertyPanel::RenderColorProperty(Property& prop, Rectangle bounds) {
    ::Color value = std::get<::Color>(prop.value);
    
    // Draw color preview
    Rectangle colorRect = {bounds.x, bounds.y + 2, 20, bounds.height - 4};
    DrawRectangleRec(colorRect, value);
    DrawRectangleLinesEx(colorRect, 1.0f, BORDER_COLOR);
    
    // Draw color values
    std::ostringstream oss;
    oss << "R:" << (int)value.r << " G:" << (int)value.g << " B:" << (int)value.b;
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), oss.str().c_str(), {bounds.x + 24, bounds.y + 2}, 14, 1.2f, TEXT_COLOR);
    } else {
        DrawText(oss.str().c_str(), (int)(bounds.x + 24), (int)(bounds.y + 2), 14, TEXT_COLOR);
    }
}

void PropertyPanel::RenderEnumProperty(Property& prop, Rectangle bounds) {
    int propIndex = (&prop - &properties[0]);
    bool isDropdownActive = dropdown.active && dropdown.propertyIndex == propIndex;
    
    // Draw main dropdown button
    Color bgColor = isDropdownActive ? INPUT_ACTIVE_COLOR : INPUT_BG_COLOR;
    DrawRectangleRec(bounds, bgColor);
    DrawRectangleLinesEx(bounds, 1.0f, isDropdownActive ? ACCENT_COLOR : BORDER_COLOR);
    
    std::string value = std::get<std::string>(prop.value);
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), value.c_str(), {bounds.x + 4, bounds.y + 2}, 18, 1.2f, TEXT_COLOR);
    } else {
        DrawText(value.c_str(), (int)(bounds.x + 4), (int)(bounds.y + 2), 18, TEXT_COLOR);
    }
    
    // Draw dropdown arrow
    const char* arrow = isDropdownActive ? "^" : "v";
    if (GuiManager::IsCustomFontLoaded()) {
        DrawTextEx(GuiManager::GetCustomFont(), arrow, {bounds.x + bounds.width - 16, bounds.y + 2}, 18, 1.2f, LABEL_COLOR);
    } else {
        DrawText(arrow, (int)(bounds.x + bounds.width - 16), (int)(bounds.y + 2), 18, LABEL_COLOR);
    }
    
    // Handle click to toggle dropdown
    if (CheckCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (isDropdownActive) {
            dropdown.active = false;
        } else {
            dropdown.active = true;
            dropdown.propertyIndex = propIndex;
            dropdown.bounds = {bounds.x, bounds.y + bounds.height, bounds.width, (float)(prop.enumOptions.size() * PROPERTY_HEIGHT)};
            dropdown.selectedIndex = -1;
            
            // Find current selection index
            for (int i = 0; i < (int)prop.enumOptions.size(); i++) {
                if (prop.enumOptions[i] == value) {
                    dropdown.selectedIndex = i;
                    break;
                }
            }
        }
    }
}

void PropertyPanel::BuildPropertiesForInstance(std::shared_ptr<Instance> instance) {
    if (!instance) return;
    
    AddBasicInstanceProperties(instance);
    AddSpecificProperties(instance);
}

void PropertyPanel::AddBasicInstanceProperties(std::shared_ptr<Instance> instance) {
    // Name property
    Property nameProp;
    nameProp.name = "Name";
    nameProp.displayName = "Name";
    nameProp.type = PropertyType::String;
    nameProp.value = instance->Name;
    nameProp.setter = [instance, this]() {
        if (auto* strVal = std::get_if<std::string>(&properties[0].value)) {
            instance->SetName(*strVal);
        }
    };
    properties.push_back(nameProp);
    
    // ClassName property (read-only)
    Property classProp;
    classProp.name = "ClassName";
    classProp.displayName = "ClassName";
    classProp.type = PropertyType::String;
    classProp.value = instance->GetClassName();
    classProp.readOnly = true;
    properties.push_back(classProp);
}

void PropertyPanel::AddSpecificProperties(std::shared_ptr<Instance> instance) {
    // Add properties specific to Workspace instances
    if (auto workspace = std::dynamic_pointer_cast<Workspace>(instance)) {
        // CurrentCamera property (read-only reference)
        Property currentCameraProp;
        currentCameraProp.name = "CurrentCamera";
        currentCameraProp.displayName = "CurrentCamera";
        currentCameraProp.type = PropertyType::String;
        
        if (workspace->camera) {
            currentCameraProp.value = workspace->camera->Name;
        } else {
            currentCameraProp.value = std::string("nil");
        }
        currentCameraProp.readOnly = true; // Read-only property
        properties.push_back(currentCameraProp);
        
        return; // Workspace instances have their specific properties
    }
    
    // Add properties specific to Camera instances
    if (auto camera = std::dynamic_pointer_cast<CameraGame>(instance)) {
        // CameraType property
        Property cameraTypeProp;
        cameraTypeProp.name = "CameraType";
        cameraTypeProp.displayName = "CameraType";
        cameraTypeProp.type = PropertyType::Enum;
        cameraTypeProp.enumOptions = {"Fixed", "Attach", "Watch", "Track", "Follow", "Custom", "Scriptable"};
        
        int cameraTypeIndex = static_cast<int>(camera->CameraType);
        if (cameraTypeIndex >= 0 && cameraTypeIndex < (int)cameraTypeProp.enumOptions.size()) {
            cameraTypeProp.value = cameraTypeProp.enumOptions[cameraTypeIndex];
        } else {
            cameraTypeProp.value = std::string("Custom");
        }
        
        cameraTypeProp.setter = [camera, this]() {
            for (auto& prop : properties) {
                if (prop.name == "CameraType") {
                    if (auto* strVal = std::get_if<std::string>(&prop.value)) {
                        for (int i = 0; i < (int)prop.enumOptions.size(); i++) {
                            if (prop.enumOptions[i] == *strVal) {
                                camera->CameraType = static_cast<::CameraType>(i);
                                camera->NotifyPropertyChanged();
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        };
        properties.push_back(cameraTypeProp);
        
        // FieldOfView property
        Property fovProp;
        fovProp.name = "FieldOfView";
        fovProp.displayName = "FieldOfView";
        fovProp.type = PropertyType::Number;
        fovProp.value = camera->FieldOfView;
        fovProp.setter = [camera, this]() {
            for (auto& prop : properties) {
                if (prop.name == "FieldOfView") {
                    if (auto* floatVal = std::get_if<float>(&prop.value)) {
                        camera->FieldOfView = Clamp(*floatVal, 1.0f, 120.0f);
                        camera->UpdateDerivedFOV();
                        camera->NotifyPropertyChanged();
                    }
                    break;
                }
            }
        };
        properties.push_back(fovProp);
        
        // HeadLocked property
        Property headLockedProp;
        headLockedProp.name = "HeadLocked";
        headLockedProp.displayName = "HeadLocked";
        headLockedProp.type = PropertyType::Boolean;
        headLockedProp.value = camera->HeadLocked;
        headLockedProp.setter = [camera, this]() {
            for (auto& prop : properties) {
                if (prop.name == "HeadLocked") {
                    if (auto* boolVal = std::get_if<bool>(&prop.value)) {
                        camera->HeadLocked = *boolVal;
                        camera->NotifyPropertyChanged();
                    }
                    break;
                }
            }
        };
        properties.push_back(headLockedProp);
        
        // HeadScale property
        Property headScaleProp;
        headScaleProp.name = "HeadScale";
        headScaleProp.displayName = "HeadScale";
        headScaleProp.type = PropertyType::Number;
        headScaleProp.value = camera->HeadScale;
        headScaleProp.setter = [camera, this]() {
            for (auto& prop : properties) {
                if (prop.name == "HeadScale") {
                    if (auto* floatVal = std::get_if<float>(&prop.value)) {
                        camera->HeadScale = Clamp(*floatVal, 0.5f, 4.0f);
                        camera->NotifyPropertyChanged();
                    }
                    break;
                }
            }
        };
        properties.push_back(headScaleProp);
        
        // VRTiltAndRollEnabled property
        Property vrTiltProp;
        vrTiltProp.name = "VRTiltAndRollEnabled";
        vrTiltProp.displayName = "VRTiltAndRollEnabled";
        vrTiltProp.type = PropertyType::Boolean;
        vrTiltProp.value = camera->VRTiltAndRollEnabled;
        vrTiltProp.setter = [camera, this]() {
            for (auto& prop : properties) {
                if (prop.name == "VRTiltAndRollEnabled") {
                    if (auto* boolVal = std::get_if<bool>(&prop.value)) {
                        camera->VRTiltAndRollEnabled = *boolVal;
                        camera->NotifyPropertyChanged();
                    }
                    break;
                }
            }
        };
        properties.push_back(vrTiltProp);
        
        return; // Camera instances have their specific properties
    }
    
    // Add properties specific to Model instances
    if (auto model = std::dynamic_pointer_cast<ModelInstance>(instance)) {
        // PrimaryPart property (Object Input)
        Property primaryPartProp;
        primaryPartProp.name = "PrimaryPart";
        primaryPartProp.displayName = "PrimaryPart";
        primaryPartProp.type = PropertyType::String;
        
        if (auto primaryPart = model->PrimaryPart.lock()) {
            primaryPartProp.value = primaryPart->Name;
        } else {
            primaryPartProp.value = std::string("nil");
        }
        primaryPartProp.readOnly = true; // For now, make it read-only in GUI
        properties.push_back(primaryPartProp);
        
        return; // Model instances only have basic properties + PrimaryPart
    }
    
    // Folder instances only have basic Instance properties (Name, Parent)
    if (std::dynamic_pointer_cast<FolderInstance>(instance)) {
        return; // No additional properties for Folder
    }
    
    // Add properties specific to BasePart instances
    if (auto basePart = std::dynamic_pointer_cast<BasePart>(instance)) {
        // Shape property (only for Part, not MeshPart)
        if (basePart->GetClassName() == "Part") {
            Property shapeProp;
            shapeProp.name = "Shape";
            shapeProp.displayName = "Shape";
            shapeProp.type = PropertyType::Enum;
            shapeProp.enumOptions = {"Ball", "Block", "Cylinder", "Wedge", "CornerWedge"};
            
            // Set current value based on Shape integer
            if (basePart->Shape >= 0 && basePart->Shape < (int)shapeProp.enumOptions.size()) {
                shapeProp.value = shapeProp.enumOptions[basePart->Shape];
            } else {
                shapeProp.value = std::string("Block"); // Default
            }
            
            shapeProp.setter = [basePart, this]() {
                for (auto& prop : properties) {
                    if (prop.name == "Shape") {
                        if (auto* strVal = std::get_if<std::string>(&prop.value)) {
                            // Convert string back to integer
                            for (int i = 0; i < (int)prop.enumOptions.size(); i++) {
                                if (prop.enumOptions[i] == *strVal) {
                                    basePart->Shape = i;
                                    basePart->ApplyShapeConstraints();
                                    // Refresh properties to show updated Size if constraints were applied
                                    RefreshProperties();
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
            };
            properties.push_back(shapeProp);
        }
        
        // Position property (from CFrame)
        Property posProp;
        posProp.name = "Position";
        posProp.displayName = "Position";
        posProp.type = PropertyType::Vector3;
        posProp.value = basePart->CF.p.toRay();
        posProp.setter = [basePart, this]() {
            for (auto& prop : properties) {
                if (prop.name == "Position") {
                    if (auto* vec3Val = std::get_if<::Vector3>(&prop.value)) {
                        basePart->CF.p = Vector3Game::fromRay(*vec3Val);
                    }
                    break;
                }
            }
        };
        properties.push_back(posProp);
        
        // Orientation property (from CFrame rotation)
        Property orientProp;
        orientProp.name = "Orientation";
        orientProp.displayName = "Orientation";
        orientProp.type = PropertyType::Vector3;
        
        // Extract orientation from CFrame (in degrees)
        float rx, ry, rz;
        basePart->CF.toOrientation(rx, ry, rz);
        // Convert from radians to degrees for display
        orientProp.value = ::Vector3{rx * 180.0f / PI, ry * 180.0f / PI, rz * 180.0f / PI};
        
        orientProp.setter = [basePart, this]() {
            for (auto& prop : properties) {
                if (prop.name == "Orientation") {
                    if (auto* vec3Val = std::get_if<::Vector3>(&prop.value)) {
                        // Convert from degrees to radians
                        float rx = vec3Val->x * PI / 180.0f;
                        float ry = vec3Val->y * PI / 180.0f;
                        float rz = vec3Val->z * PI / 180.0f;
                        
                        // Create new CFrame with same position but new orientation
                        CFrame newCF = CFrame::fromOrientation(rx, ry, rz);
                        newCF.p = basePart->CF.p; // Keep the same position
                        basePart->CF = newCF;
                    }
                    break;
                }
            }
        };
        properties.push_back(orientProp);
        
        // Size property
        Property sizeProp;
        sizeProp.name = "Size";
        sizeProp.displayName = "Size";
        sizeProp.type = PropertyType::Vector3;
        sizeProp.value = basePart->Size;
        sizeProp.setter = [basePart, this]() {
            // Find the Size property in the list
            for (auto& prop : properties) {
                if (prop.name == "Size") {
                    if (auto* vec3Val = std::get_if<::Vector3>(&prop.value)) {
                        basePart->Size = *vec3Val;
                    }
                    break;
                }
            }
        };
        properties.push_back(sizeProp);
        
        // Color property
        Property colorProp;
        colorProp.name = "Color";
        colorProp.displayName = "Color";
        colorProp.type = PropertyType::Color;
        
        // Convert Color3 to raylib Color
        ::Color raylibColor = {
            (unsigned char)(basePart->Color.r * 255),
            (unsigned char)(basePart->Color.g * 255),
            (unsigned char)(basePart->Color.b * 255),
            255
        };
        colorProp.value = raylibColor;
        colorProp.setter = [basePart, this]() {
            for (auto& prop : properties) {
                if (prop.name == "Color") {
                    if (auto* colorVal = std::get_if<::Color>(&prop.value)) {
                        basePart->Color.r = colorVal->r / 255.0f;
                        basePart->Color.g = colorVal->g / 255.0f;
                        basePart->Color.b = colorVal->b / 255.0f;
                    }
                    break;
                }
            }
        };
        properties.push_back(colorProp);
        
        // Transparency property
        Property transProp;
        transProp.name = "Transparency";
        transProp.displayName = "Transparency";
        transProp.type = PropertyType::Number;
        transProp.value = basePart->Transparency;
        transProp.setter = [basePart, this]() {
            for (auto& prop : properties) {
                if (prop.name == "Transparency") {
                    if (auto* floatVal = std::get_if<float>(&prop.value)) {
                        basePart->Transparency = Clamp(*floatVal, 0.0f, 1.0f);
                    }
                    break;
                }
            }
        };
        properties.push_back(transProp);
        
        // Anchored property
        Property anchoredProp;
        anchoredProp.name = "Anchored";
        anchoredProp.displayName = "Anchored";
        anchoredProp.type = PropertyType::Boolean;
        anchoredProp.value = basePart->Anchored;
        anchoredProp.setter = [basePart, this]() {
            for (auto& prop : properties) {
                if (prop.name == "Anchored") {
                    if (auto* boolVal = std::get_if<bool>(&prop.value)) {
                        basePart->Anchored = *boolVal;
                    }
                    break;
                }
            }
        };
        properties.push_back(anchoredProp);
        
        // CanCollide property
        Property canCollideProp;
        canCollideProp.name = "CanCollide";
        canCollideProp.displayName = "CanCollide";
        canCollideProp.type = PropertyType::Boolean;
        canCollideProp.value = basePart->CanCollide;
        canCollideProp.setter = [basePart, this]() {
            for (auto& prop : properties) {
                if (prop.name == "CanCollide") {
                    if (auto* boolVal = std::get_if<bool>(&prop.value)) {
                        basePart->CanCollide = *boolVal;
                    }
                    break;
                }
            }
        };
        properties.push_back(canCollideProp);
    }
}

bool PropertyPanel::HandlePropertyInput(Property& prop, Rectangle bounds) {
    // This would handle input for editable properties
    return false;
}

void PropertyPanel::ApplyPropertyChange(Property& prop) {
    if (prop.setter) {
        prop.setter();
    }
}

void PropertyPanel::StartTextInput(int propertyIndex, const std::string& initialValue, const std::string& fieldName) {
    textInput.active = true;
    textInput.buffer = initialValue;
    textInput.cursorPos = (int)initialValue.length();
    textInput.propertyIndex = propertyIndex;
    textInput.fieldName = fieldName;
    textInput.blinkTimer = 0.0;
}

void PropertyPanel::UpdateTextInput() {
    if (!textInput.active) return;
    
    textInput.blinkTimer += GetFrameTime();
    
    // Handle text input
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125) {
            textInput.buffer.insert(textInput.cursorPos, 1, (char)key);
            textInput.cursorPos++;
        }
        key = GetCharPressed();
    }
    
    // Handle special keys
    if (IsKeyPressed(KEY_BACKSPACE) && textInput.cursorPos > 0) {
        textInput.buffer.erase(textInput.cursorPos - 1, 1);
        textInput.cursorPos--;
    }
    
    if (IsKeyPressed(KEY_DELETE) && textInput.cursorPos < (int)textInput.buffer.length()) {
        textInput.buffer.erase(textInput.cursorPos, 1);
    }
    
    if (IsKeyPressed(KEY_LEFT) && textInput.cursorPos > 0) {
        textInput.cursorPos--;
    }
    
    if (IsKeyPressed(KEY_RIGHT) && textInput.cursorPos < (int)textInput.buffer.length()) {
        textInput.cursorPos++;
    }
    
    if (IsKeyPressed(KEY_ENTER)) {
        EndTextInput(true);
    }
    
    if (IsKeyPressed(KEY_ESCAPE)) {
        EndTextInput(false);
    }
}

void PropertyPanel::EndTextInput(bool apply) {
    if (!textInput.active) return;
    
    if (apply && textInput.propertyIndex >= 0 && textInput.propertyIndex < (int)properties.size()) {
        Property& prop = properties[textInput.propertyIndex];
        
        if (prop.type == PropertyType::String) {
            prop.value = textInput.buffer;
            ApplyPropertyChange(prop);
        } else if (prop.type == PropertyType::Number) {
            try {
                float value = std::stof(textInput.buffer);
                prop.value = value;
                ApplyPropertyChange(prop);
            } catch (...) {
                // Invalid number, ignore
            }
        }
    }
    
    textInput.active = false;
    textInput.buffer.clear();
    textInput.propertyIndex = -1;
    textInput.fieldName.clear();
}
