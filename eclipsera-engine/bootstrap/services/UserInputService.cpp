#include "bootstrap/services/UserInputService.h"
#include "bootstrap/LuaScheduler.h"
#include "bootstrap/Game.h"
#include "core/logging/Logging.h"
#include "core/datatypes/Enum.h"
#include "core/datatypes/Vector2.h"
#include "raylib.h"
#include "lua.h"
#include "lualib.h"
#include <cstring>
#include <unordered_map>

extern void Lua_PushSignal(lua_State* L, const std::shared_ptr<RTScriptSignal>& sig);

// Key mapping helper
static const std::unordered_map<int, const char*> keyNameMap = {
    {KEY_A, "A"}, {KEY_B, "B"}, {KEY_C, "C"}, {KEY_D, "D"},
    {KEY_E, "E"}, {KEY_F, "F"}, {KEY_G, "G"}, {KEY_H, "H"},
    {KEY_I, "I"}, {KEY_J, "J"}, {KEY_K, "K"}, {KEY_L, "L"},
    {KEY_M, "M"}, {KEY_N, "N"}, {KEY_O, "O"}, {KEY_P, "P"},
    {KEY_Q, "Q"}, {KEY_R, "R"}, {KEY_S, "S"}, {KEY_T, "T"},
    {KEY_U, "U"}, {KEY_V, "V"}, {KEY_W, "W"}, {KEY_X, "X"},
    {KEY_Y, "Y"}, {KEY_Z, "Z"},
    {KEY_ZERO, "Zero"}, {KEY_ONE, "One"}, {KEY_TWO, "Two"},
    {KEY_THREE, "Three"}, {KEY_FOUR, "Four"}, {KEY_FIVE, "Five"},
    {KEY_SIX, "Six"}, {KEY_SEVEN, "Seven"}, {KEY_EIGHT, "Eight"},
    {KEY_NINE, "Nine"},
    {KEY_SPACE, "Space"},
    {KEY_ENTER, "Return"},
    {KEY_ESCAPE, "Escape"},
    {KEY_BACKSPACE, "Backspace"},
    {KEY_RIGHT, "Right"},
    {KEY_LEFT, "Left"},
    {KEY_DOWN, "Down"},
    {KEY_UP, "Up"},
    {KEY_LEFT_SHIFT, "LeftShift"},
    {KEY_RIGHT_SHIFT, "RightShift"},
    {KEY_LEFT_CONTROL, "LeftControl"},
    {KEY_RIGHT_CONTROL, "RightControl"},
    {KEY_LEFT_ALT, "LeftAlt"},
    {KEY_RIGHT_ALT, "RightAlt"}
};

// Lua function for GetMouseDelta method
static int GetMouseDeltaLua(lua_State* L) {
    // Get the UserInputService instance from the global service
    auto uis = std::dynamic_pointer_cast<UserInputService>(Service::Get("UserInputService"));
    
    if (uis) {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, uis->mouseDelta.x);
        lua_setfield(L, -2, "X");
        lua_pushnumber(L, uis->mouseDelta.y);
        lua_setfield(L, -2, "Y");
        return 1; // Return the Vector2 table
    }
    
    // Return zero delta if no service found
    lua_createtable(L, 0, 2);
    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "X");
    lua_pushnumber(L, 0);
    lua_setfield(L, -2, "Y");
    return 1;
}

// Lua function for GetMouseLocation method
static int GetMouseLocationLua(lua_State* L) {
    Vector2 mousePos = GetMousePosition();
    // Use the proper Vector2Game class with arithmetic support
    Vector2Game v2(mousePos.x, mousePos.y);
    lb::push(L, v2);
    return 1; // Return the Vector2Game object
}

// Lua function for IsMouseButtonPressed method
static int IsMouseButtonPressedLua(lua_State* L) {
    auto uis = std::dynamic_pointer_cast<UserInputService>(Service::Get("UserInputService"));
    if (!uis) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    // Check if argument is an enum table with Value field
    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "Value");
        if (lua_isnumber(L, -1)) {
            int mouseButton = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
            lua_pushboolean(L, uis->IsMouseButtonPressed(mouseButton));
            return 1;
        }
        lua_pop(L, 1);
    }
    
    // Fallback: treat as direct number
    if (lua_isnumber(L, 1)) {
        int mouseButton = (int)lua_tonumber(L, 1);
        lua_pushboolean(L, uis->IsMouseButtonPressed(mouseButton));
        return 1;
    }
    
    lua_pushboolean(L, false);
    return 1;
}

// Lua function for IsKeyDown method
static int IsKeyDownLua(lua_State* L) {
    auto uis = std::dynamic_pointer_cast<UserInputService>(Service::Get("UserInputService"));
    if (!uis) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    // Check if argument is an enum table with Value field
    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "Value");
        if (lua_isnumber(L, -1)) {
            int keyCode = (int)lua_tonumber(L, -1);
            lua_pop(L, 1);
            lua_pushboolean(L, uis->IsKeyDown(keyCode));
            return 1;
        }
        lua_pop(L, 1);
    }
    
    // Fallback: treat as direct number
    if (lua_isnumber(L, 1)) {
        int keyCode = (int)lua_tonumber(L, 1);
        lua_pushboolean(L, uis->IsKeyDown(keyCode));
        return 1;
    }
    
    lua_pushboolean(L, false);
    return 1;
}

UserInputService::UserInputService() : Service("UserInputService", InstanceClass::UserInputService) {}

void UserInputService::EnsureSignals() const {
    auto* self = const_cast<UserInputService*>(this);
    LuaScheduler* sch = (g_game && g_game->luaScheduler) ? g_game->luaScheduler.get() : nullptr;
    if (!self->InputBegan)     self->InputBegan     = std::make_shared<RTScriptSignal>(sch);
    if (!self->InputEnded)     self->InputEnded     = std::make_shared<RTScriptSignal>(sch);
    if (!self->InputChanged)   self->InputChanged   = std::make_shared<RTScriptSignal>(sch);
    if (!self->MouseMoved)     self->MouseMoved     = std::make_shared<RTScriptSignal>(sch);
}

bool UserInputService::LuaGet(lua_State* L, const char* k) const {
    EnsureSignals();
    
    if (!strcmp(k, "InputBegan"))     { Lua_PushSignal(L, InputBegan); return true; }
    if (!strcmp(k, "InputEnded"))     { Lua_PushSignal(L, InputEnded); return true; }
    if (!strcmp(k, "InputChanged"))   { Lua_PushSignal(L, InputChanged); return true; }
    if (!strcmp(k, "MouseMoved"))     { Lua_PushSignal(L, MouseMoved); return true; }
    
    if (!strcmp(k, "MousePosition")) {
        Vector2 mousePos = GetMousePosition();
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, mousePos.x);
        lua_setfield(L, -2, "X");
        lua_pushnumber(L, mousePos.y);
        lua_setfield(L, -2, "Y");
        return true;
    }

    if (!strcmp(k, "GetMouseDelta")) {
        // Return a function that returns the mouse delta
        lua_pushcfunction(L, GetMouseDeltaLua, nullptr);
        return true;
    }

    if (!strcmp(k, "GetMouseLocation")) {
        // Return a function that returns the mouse location
        lua_pushcfunction(L, GetMouseLocationLua, nullptr);
        return true;
    }
    
    if (!strcmp(k, "IsMouseButtonPressed")) {
        lua_pushcfunction(L, IsMouseButtonPressedLua, nullptr);
        return true;
    }
    
    if (!strcmp(k, "IsKeyDown")) {
        lua_pushcfunction(L, IsKeyDownLua, nullptr);
        return true;
    }

    if (!strcmp(k, "MouseBehavior")) {
        // Create MouseBehavior enum item
        lua_createtable(L, 0, 2);
        lua_pushstring(L, mouseBehavior == MouseBehavior::Default ? "Default" : 
                           mouseBehavior == MouseBehavior::LockCenter ? "LockCenter" : "LockCurrentPosition");
        lua_setfield(L, -2, "Name");
        lua_pushnumber(L, static_cast<int>(mouseBehavior));
        lua_setfield(L, -2, "Value");
        return true;
    }

    if (!strcmp(k, "MouseEnabled"))   { lua_pushboolean(L, true); return true; }
    if (!strcmp(k, "KeyboardEnabled")) { lua_pushboolean(L, true); return true; }
    if (!strcmp(k, "MouseIconEnabled")) { lua_pushboolean(L, !IsCursorHidden()); return true; }
    
    return false;
}

bool UserInputService::LuaSet(lua_State* L, const char* k, int idx) {
    if (!strcmp(k, "MouseIconEnabled")) {
        bool enabled = lua_toboolean(L, idx);
        if (enabled) ShowCursor(); else HideCursor();
        return true;
    }
    
    if (!strcmp(k, "MouseBehavior")) {
        if (lua_istable(L, idx)) {
            lua_getfield(L, idx, "Value");
            if (lua_isnumber(L, -1)) {
                int value = (int)lua_tonumber(L, -1);
                if (value >= 0 && value <= 2) {
                    MouseBehavior newBehavior = static_cast<MouseBehavior>(value);
                    if (mouseBehavior != newBehavior) {
                        mouseBehavior = newBehavior;
                        if (mouseBehavior == MouseBehavior::LockCurrentPosition) {
                            lockedMousePosition = GetMousePosition();
                        } else if (mouseBehavior == MouseBehavior::LockCenter) {
                            lockedMousePosition = {(float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f};
                        }
                        mousePositionLocked = (mouseBehavior != MouseBehavior::Default);
                    }
                }
            }
            lua_pop(L, 1);
        }
        return true;
    }
    
    return false;
}

void UserInputService::PushInputObject(lua_State* L, const char* inputType, const char* keyName, Vector2 mousePos) {
    lua_createtable(L, 0, 4);
    
    // UserInputType - get from global Enum table to ensure same instance
    lua_getglobal(L, "Enum");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "UserInputType");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, inputType);
            if (!lua_isnil(L, -1)) {
                // Found the enum item in global table, use it
                lua_setfield(L, -4, "UserInputType");
                lua_pop(L, 2); // pop UserInputType table and Enum table
            } else {
                // Fallback
                lua_pop(L, 3); // pop nil, UserInputType table, and Enum table
                lua_createtable(L, 0, 1);
                lua_pushstring(L, inputType);
                lua_setfield(L, -2, "Name");
                lua_setfield(L, -2, "UserInputType");
            }
        } else {
            lua_pop(L, 2); // pop UserInputType and Enum
            lua_createtable(L, 0, 1);
            lua_pushstring(L, inputType);
            lua_setfield(L, -2, "Name");
            lua_setfield(L, -2, "UserInputType");
        }
    } else {
        lua_pop(L, 1); // pop Enum
        lua_createtable(L, 0, 1);
        lua_pushstring(L, inputType);
        lua_setfield(L, -2, "Name");
        lua_setfield(L, -2, "UserInputType");
    }
    
    // KeyCode - only set for keyboard inputs, not mouse buttons
    if (strlen(keyName) > 0 && strcmp(inputType, "MouseButton") != 0) {
        lua_getglobal(L, "Enum");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "KeyCode");
            if (lua_istable(L, -1)) {
                lua_getfield(L, -1, keyName);
                if (!lua_isnil(L, -1)) {
                    // Found the enum item in global table, use it
                    lua_setfield(L, -4, "KeyCode");
                    lua_pop(L, 2); // pop KeyCode table and Enum table
                } else {
                    // Fallback
                    lua_pop(L, 3); // pop nil, KeyCode table, and Enum table
                    lua_createtable(L, 0, 1);
                    lua_pushstring(L, keyName);
                    lua_setfield(L, -2, "Name");
                    lua_setfield(L, -2, "KeyCode");
                }
            } else {
                lua_pop(L, 2); // pop KeyCode and Enum
                lua_createtable(L, 0, 1);
                lua_pushstring(L, keyName);
                lua_setfield(L, -2, "Name");
                lua_setfield(L, -2, "KeyCode");
            }
        } else {
            lua_pop(L, 1); // pop Enum
            lua_createtable(L, 0, 1);
            lua_pushstring(L, keyName);
            lua_setfield(L, -2, "Name");
            lua_setfield(L, -2, "KeyCode");
        }
    } else {
        // For mouse buttons or empty keyName, set KeyCode to Unknown
        lua_getglobal(L, "Enum");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "KeyCode");
            if (lua_istable(L, -1)) {
                lua_getfield(L, -1, "Unknown");
                if (!lua_isnil(L, -1)) {
                    lua_setfield(L, -4, "KeyCode");
                    lua_pop(L, 2); // pop KeyCode table and Enum table
                } else {
                    lua_pop(L, 3); // pop nil, KeyCode table, and Enum table
                    lua_createtable(L, 0, 1);
                    lua_pushstring(L, "Unknown");
                    lua_setfield(L, -2, "Name");
                    lua_setfield(L, -2, "KeyCode");
                }
            } else {
                lua_pop(L, 2); // pop KeyCode and Enum
                lua_createtable(L, 0, 1);
                lua_pushstring(L, "Unknown");
                lua_setfield(L, -2, "Name");
                lua_setfield(L, -2, "KeyCode");
            }
        } else {
            lua_pop(L, 1); // pop Enum
            lua_createtable(L, 0, 1);
            lua_pushstring(L, "Unknown");
            lua_setfield(L, -2, "Name");
            lua_setfield(L, -2, "KeyCode");
        }
    }
    
    // Position
    lua_createtable(L, 0, 2);
    lua_pushnumber(L, mousePos.x);
    lua_setfield(L, -2, "X");
    lua_pushnumber(L, mousePos.y);
    lua_setfield(L, -2, "Y");
    lua_setfield(L, -2, "Position");
    
    // Delta - only for mouse movement inputs
    if (strcmp(inputType, "MouseMovement") == 0) {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, mouseDelta.x);
        lua_setfield(L, -2, "X");
        lua_pushnumber(L, mouseDelta.y);
        lua_setfield(L, -2, "Y");
        lua_setfield(L, -2, "Delta");
    }
}

void UserInputService::ApplyMouseBehavior() {
    if (mousePositionLocked) {
        SetMousePosition((int)lockedMousePosition.x, (int)lockedMousePosition.y);
    }
}

void UserInputService::Update() {
    Vector2 mousePos = GetMousePosition();
    
    // Calculate mouse delta before applying mouse behavior
    if (firstMouseUpdate) {
        lastFrameMousePos = mousePos;
        mouseDelta = {0, 0};
        firstMouseUpdate = false;
    } else {
        mouseDelta.x = mousePos.x - lastFrameMousePos.x;
        mouseDelta.y = mousePos.y - lastFrameMousePos.y;
    }
    
    // Apply mouse behavior (this may reset mouse position)
    ApplyMouseBehavior();
    
    // Update lastFrameMousePos to the position AFTER applying behavior
    // This ensures next frame's delta calculation is correct
    lastFrameMousePos = GetMousePosition();
    
    // Update mousePos after applying behavior for position-based events
    mousePos = GetMousePosition();

    // Handle mouse movement
    if (lastMouseX != mousePos.x || lastMouseY != mousePos.y) {
        if (MouseMoved) {
            lua_State* L = g_game->luaScheduler->GetMainState();
            int top = lua_gettop(L);  // Save current stack top
            PushInputObject(L, "MouseMovement", "", mousePos);
            MouseMoved->Fire(L, top + 1, 1);  // Use absolute indices from saved top
            lua_settop(L, top);  // Restore stack
        }
        lastMouseX = mousePos.x;
        lastMouseY = mousePos.y;
    }

    // Handle keyboard
    for (const auto& [key, name] : keyNameMap) {
        if (IsKeyPressed(key)) {
            if (InputBegan) {
                lua_State* L = g_game->luaScheduler->GetMainState();
                if (L) {
                    int top = lua_gettop(L);  // Save current stack top
                    PushInputObject(L, "Keyboard", name, mousePos);
                    lua_pushboolean(L, false);
                    InputBegan->Fire(L, top + 1, 2);  // Use absolute indices from saved top
                    lua_settop(L, top);  // Restore stack
                }
            }
        }
        if (IsKeyReleased(key)) {
            if (InputEnded) {
                lua_State* L = g_game->luaScheduler->GetMainState();
                int top = lua_gettop(L);  // Save current stack top
                PushInputObject(L, "Keyboard", name, mousePos);
                lua_pushboolean(L, false);
                InputEnded->Fire(L, top + 1, 2);  // Use absolute indices from saved top
                lua_settop(L, top);  // Restore stack
            }
        }
    }

    // Handle mouse buttons
    for (int button = 0; button < 3; button++) {
        if (IsMouseButtonPressed(button)) {
            if (InputBegan) {
                lua_State* L = g_game->luaScheduler->GetMainState();
                int top = lua_gettop(L);  // Save current stack top
                const char* buttonName;
                switch (button) {
                    case 0: buttonName = "MouseButton1"; break;
                    case 1: buttonName = "MouseButton2"; break;
                    case 2: buttonName = "MouseButton3"; break;
                    default: buttonName = "Unknown"; break;
                }
                PushInputObject(L, buttonName, "", mousePos);
                lua_pushboolean(L, false);
                InputBegan->Fire(L, top + 1, 2);  // Use absolute indices from saved top
                lua_settop(L, top);  // Restore stack
            }
        }
        if (IsMouseButtonReleased(button)) {
            if (InputEnded) {
                lua_State* L = g_game->luaScheduler->GetMainState();
                int top = lua_gettop(L);  // Save current stack top
                const char* buttonName;
                switch (button) {
                    case 0: buttonName = "MouseButton1"; break;
                    case 1: buttonName = "MouseButton2"; break;
                    case 2: buttonName = "MouseButton3"; break;
                    default: buttonName = "Unknown"; break;
                }
                PushInputObject(L, buttonName, "", mousePos);
                lua_pushboolean(L, false);
                InputEnded->Fire(L, top + 1, 2);  // Use absolute indices from saved top
                lua_settop(L, top);  // Restore stack
            }
        }
    }
}

// Implementation of input state checking methods
bool UserInputService::IsMouseButtonPressed(int mouseButton) const {
    // Convert from Roblox mouse button enum to raylib mouse button
    switch (mouseButton) {
        case 0: return ::IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        case 1: return ::IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
        case 2: return ::IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        default: return false;
    }
}

bool UserInputService::IsKeyDown(int keyCode) const {
    // Use raylib's IsKeyDown function directly
    // The keyCode should match raylib's key constants
    return ::IsKeyDown(keyCode);
}

static Instance::Registrar s_regUserInputService("UserInputService", [] {
    return std::make_shared<UserInputService>();
});
