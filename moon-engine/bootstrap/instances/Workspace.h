#pragma once
#include "bootstrap/services/Service.h"
#include "core/datatypes/Vector3Game.h"
#include <vector>
#include <memory>

struct BasePart;
struct CameraGame;
struct BaseScript;

enum class RunContext;

struct RaycastParams {
    Vector3Game Origin;
    Vector3Game Direction;
    float MaxDistance = 1000.0f;
    std::vector<std::shared_ptr<BasePart>> FilterDescendantsInstances;
    bool FilterType = false; // false = Blacklist, true = Whitelist
};

struct RaycastResult {
    bool Hit = false;
    Vector3Game Position;
    Vector3Game Normal;
    std::shared_ptr<BasePart> Instance;
    float Distance = 0.0f;
};

struct Workspace : Service {
    std::shared_ptr<CameraGame> camera; // CurrentCamera
    std::vector<std::shared_ptr<BasePart>> parts;

    explicit Workspace(std::string name = "Workspace");
    ~Workspace() override;
    
    bool LuaGet(lua_State* L, const char* k) const override;
    
    // Raycast functionality
    RaycastResult Raycast(const RaycastParams& params) const;
    
    // Camera synchronization
    void SynchronizeCameras();
    void RegisterCameraForSync(std::shared_ptr<CameraGame> cam);
    void UnregisterCameraFromSync(std::shared_ptr<CameraGame> cam);
    bool IsSynchronizingCameras() const { return synchronizingCameras; }
    
    // Script context management
    void SetCurrentScriptContext(RunContext context);
    RunContext GetCurrentScriptContext() const { return currentScriptContext; }
    RunContext GetScriptContextFromLuaState(lua_State* L) const;
    bool ShouldShowCurrentCamera() const;
    bool ShouldSyncCameraChanges() const;
    bool ShouldAllowCameraModification() const;
    
private:
    bool RayIntersectsBox(const Vector3Game& rayOrigin, const Vector3Game& rayDir, 
                         const Vector3Game& boxCenter, const Vector3Game& boxSize,
                         float& distance, Vector3Game& normal) const;
    
    // Camera synchronization
    std::vector<std::shared_ptr<CameraGame>> syncedCameras;
    bool synchronizingCameras = false; // Prevent infinite recursion
    
    // Script context for camera behavior
    RunContext currentScriptContext;
    bool hasSetScriptContext = false;
};
