#pragma once
#include "bootstrap/services/Service.h"
#include "bootstrap/signals/Signal.h"
#include <memory>
#include "raylib.h"

enum class MouseBehavior {
    Default = 0,
    LockCenter = 1,
    LockCurrentPosition = 2
};

class UserInputService : public Service {
public:
    UserInputService();
    
    bool LuaGet(lua_State* L, const char* k) const override;
    bool LuaSet(lua_State* L, const char* k, int idx) override;
    void Update();

    // Input state checking methods
    bool IsMouseButtonPressed(int mouseButton) const;
    bool IsKeyDown(int keyCode) const;

    // Mouse delta tracking (public for Lua access)
    Vector2 mouseDelta = {0, 0};

private:
    void EnsureSignals() const;
    void PushInputObject(lua_State* L, const char* inputType, const char* keyName, Vector2 mousePos);
    void ApplyMouseBehavior();

    std::shared_ptr<RTScriptSignal> InputBegan;
    std::shared_ptr<RTScriptSignal> InputEnded;
    std::shared_ptr<RTScriptSignal> InputChanged;
    std::shared_ptr<RTScriptSignal> MouseMoved;

    float lastMouseX = 0;
    float lastMouseY = 0;
    
    Vector2 lastFrameMousePos = {0, 0};
    bool firstMouseUpdate = true;
    
    MouseBehavior mouseBehavior = MouseBehavior::Default;
    Vector2 lockedMousePosition = {0, 0};
    bool mousePositionLocked = false;
};
