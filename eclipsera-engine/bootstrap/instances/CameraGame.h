#pragma once
#include "bootstrap/Instance.h"
#include "core/datatypes/CFrame.h"
#include "core/datatypes/Vector3Game.h"
#include "core/datatypes/Vector2.h"
#include "core/datatypes/Ray.h"

enum class CameraType {
    Fixed = 0,
    Attach = 1,
    Watch = 2,
    Track = 3,
    Follow = 4,
    Custom = 5,
    Scriptable = 6
};

enum class FieldOfViewMode {
    Vertical = 0,
    Diagonal = 1,
    MaxAxis = 2
};

struct CameraGame : Instance {
    // Core camera properties
    CFrame CFrameValue{Vector3Game(0.0f, 10.0f, 20.0f)};
    CFrame Focus{Vector3Game(0.0f, 0.0f, 0.0f)};
    
    // Camera settings
    CameraType CameraType = CameraType::Custom;
    std::shared_ptr<Instance> CameraSubject = nullptr;
    
    // Field of view properties
    float FieldOfView = 70.0f;
    float DiagonalFieldOfView = 0.0f; // Calculated from FieldOfView
    float MaxAxisFieldOfView = 0.0f;  // Calculated from FieldOfView
    FieldOfViewMode FieldOfViewMode = FieldOfViewMode::Vertical;
    
    // VR properties
    bool HeadLocked = true;
    float HeadScale = 1.0f;
    bool VRTiltAndRollEnabled = true;
    
    // Read-only properties
    float NearPlaneZ = -0.1f;
    Vector2 ViewportSize{1280.0f, 720.0f};
    
    // Legacy properties (for compatibility)
    ::Vector3 Position{0.0f, 10.0f, 20.0f};
    ::Vector3 Target{0.0f, 0.0f, 0.0f};
    
    CameraGame(std::string name = "Camera");
    ~CameraGame() override;
    
    // Lua bindings
    bool LuaGet(lua_State* L, const char* k) const override;
    bool LuaSet(lua_State* L, const char* k, int idx) override;
    
    // Camera methods
    CFrame GetRenderCFrame() const;
    float GetRoll() const;
    void SetRoll(float rollAngle);
    
    // Screen/World conversion methods
    RayGame ScreenPointToRay(float x, float y, float depth = 1.0f) const;
    RayGame ViewportPointToRay(float x, float y, float depth = 1.0f) const;
    Vector2 WorldToScreenPoint(const Vector3Game& worldPoint) const;
    Vector2 WorldToViewportPoint(const Vector3Game& worldPoint) const;
    
    // Camera utility methods
    void ZoomToExtents(const std::vector<std::shared_ptr<Instance>>& parts);
    std::vector<std::shared_ptr<Instance>> GetPartsObscuringTarget(const Vector3Game& target, const std::vector<std::shared_ptr<Instance>>& parts) const;
    
    // Utility methods
    void UpdateViewportSize(float width, float height);
    void UpdateDerivedFOV();
    
    // Synchronization
    void NotifyPropertyChanged();
    
private:
    float rollAngle = 0.0f;
};
