#include "bootstrap/instances/Workspace.h"
#include "bootstrap/instances/Part.h"
#include "bootstrap/instances/CameraGame.h"
#include "bootstrap/instances/BaseScript.h"
#include "bootstrap/Game.h"
#include "bootstrap/ScriptingAPI.h"
#include "core/datatypes/LuaDatatypes.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstring>

extern std::shared_ptr<Game> g_game;

Workspace::Workspace(std::string name)
    : Service(std::move(name), InstanceClass::Workspace), currentScriptContext(RunContext::Server) {
    
    OnDescendantAdded([this](const std::shared_ptr<Instance>& c){
        if (c->Class == InstanceClass::Part || c->Class == InstanceClass::MeshPart) {
            // Use dynamic_pointer_cast to preserve the original type
            auto basePart = std::dynamic_pointer_cast<BasePart>(c);
            if (basePart) {
                parts.push_back(basePart);
            }
        } else if (c->Class == InstanceClass::Camera) {
            auto cameraInstance = std::static_pointer_cast<CameraGame>(c);
            // If this is the first camera and we don't have a CurrentCamera target yet, use it
            if (!camera) {
                camera = cameraInstance;
            }
        }
    });
    OnDescendantRemoved([this](const std::shared_ptr<Instance>& c){
        if (c->Class == InstanceClass::Part || c->Class == InstanceClass::MeshPart) {
            // Use dynamic_pointer_cast to preserve the original type
            auto basePart = std::dynamic_pointer_cast<BasePart>(c);
            if (basePart) {
                auto it = std::find(parts.begin(), parts.end(), basePart);
                if (it != parts.end()) { *it = parts.back(); parts.pop_back(); }
            }
        } else if (c->Class == InstanceClass::Camera) {
            auto cameraInstance = std::static_pointer_cast<CameraGame>(c);
            if (camera && camera.get() == c.get()) camera.reset();
            // Unregister from synchronization
            UnregisterCameraFromSync(cameraInstance);
        }
    });
}
Workspace::~Workspace() = default;

// Static function for Raycast Lua binding
static int WorkspaceRaycast(lua_State* L) {
    // Get workspace instance
    auto* ws = static_cast<Workspace*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!ws) {
        luaL_error(L, "Invalid workspace instance");
        return 0;
    }
    
    // Expect a table with raycast parameters
    if (!lua_istable(L, 1)) {
        luaL_error(L, "Raycast expects a table with parameters");
        return 0;
    }
    
    RaycastParams params;
    
    // Get Origin
    lua_getfield(L, 1, "Origin");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "X");
        params.Origin.x = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "Y");
        params.Origin.y = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "Z");
        params.Origin.z = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    
    // Get Direction
    lua_getfield(L, 1, "Direction");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "X");
        params.Direction.x = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "Y");
        params.Direction.y = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "Z");
        params.Direction.z = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    
    // Get MaxDistance (optional)
    lua_getfield(L, 1, "MaxDistance");
    if (lua_isnumber(L, -1)) {
        params.MaxDistance = (float)lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    
    // Perform raycast
    RaycastResult result = ws->Raycast(params);
    
    // Return result
    if (result.Hit) {
        lua_createtable(L, 0, 5);
        
        // Position
        lua_createtable(L, 0, 3);
        lua_pushnumber(L, result.Position.x);
        lua_setfield(L, -2, "X");
        lua_pushnumber(L, result.Position.y);
        lua_setfield(L, -2, "Y");
        lua_pushnumber(L, result.Position.z);
        lua_setfield(L, -2, "Z");
        lua_setfield(L, -2, "Position");
        
        // Normal
        lua_createtable(L, 0, 3);
        lua_pushnumber(L, result.Normal.x);
        lua_setfield(L, -2, "X");
        lua_pushnumber(L, result.Normal.y);
        lua_setfield(L, -2, "Y");
        lua_pushnumber(L, result.Normal.z);
        lua_setfield(L, -2, "Z");
        lua_setfield(L, -2, "Normal");
        
        // Distance
        lua_pushnumber(L, result.Distance);
        lua_setfield(L, -2, "Distance");
        
        // Instance (if available)
        if (result.Instance) {
            // Push the Part instance - this would need proper Lua binding
            lua_pushstring(L, "Part");
            lua_setfield(L, -2, "Instance");
        }
        
        return 1;
    } else {
        lua_pushnil(L);
        return 1;
    }
}

bool Workspace::LuaGet(lua_State* L, const char* k) const {
    if (!strcmp(k, "CurrentCamera")) {
        // Get the current script context from the Lua thread
        RunContext scriptContext = GetScriptContextFromLuaState(L);
        
        switch (scriptContext) {
            case RunContext::Client:
                // Client scripts can access CurrentCamera (read-only reference)
                if (camera) {
                    Lua_PushInstance(L, std::static_pointer_cast<Instance>(camera));
                } else {
                    lua_pushnil(L);
                }
                break;
            case RunContext::Server:
                // Server scripts cannot access CurrentCamera at all - property doesn't exist
                return false; // Let parent handle it (will result in nil/error)
            case RunContext::Plugin:
            default:
                // Plugin scripts and default behavior can access CurrentCamera
                if (camera) {
                    Lua_PushInstance(L, std::static_pointer_cast<Instance>(camera));
                } else {
                    lua_pushnil(L);
                }
                break;
        }
        return true;
    }
    if (!strcmp(k, "Raycast")) {
        // For now, just return nil - the Raycast functionality is implemented
        // but needs proper Lua binding integration with the existing system
        lua_pushnil(L);
        return true;
    }
    
    return Service::LuaGet(L, k);
}

RaycastResult Workspace::Raycast(const RaycastParams& params) const {
    RaycastResult result;
    float closestDistance = std::numeric_limits<float>::max();
    
    // Normalize direction
    Vector3Game rayDir = params.Direction.normalized();
    
    // Check intersection with all parts
    for (const auto& part : parts) {
        if (!part || !part->Alive) continue;
        
        // Get part position and size
        Vector3Game partPos = part->CF.p;
        Vector3Game partSize = Vector3Game::fromRay(part->Size);
        
        float distance;
        Vector3Game normal;
        
        if (RayIntersectsBox(params.Origin, rayDir, partPos, partSize, distance, normal)) {
            if (distance > 0 && distance <= params.MaxDistance && distance < closestDistance) {
                closestDistance = distance;
                result.Hit = true;
                result.Distance = distance;
                result.Position = params.Origin + rayDir * distance;
                result.Normal = normal;
                result.Instance = part;
            }
        }
    }
    
    return result;
}

bool Workspace::RayIntersectsBox(const Vector3Game& rayOrigin, const Vector3Game& rayDir, 
                                const Vector3Game& boxCenter, const Vector3Game& boxSize,
                                float& distance, Vector3Game& normal) const {
    // Calculate box bounds
    Vector3Game boxMin = boxCenter - boxSize * 0.5f;
    Vector3Game boxMax = boxCenter + boxSize * 0.5f;
    
    // Ray-box intersection using slab method
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::max();
    
    Vector3Game hitNormal(0, 0, 0);
    
    for (int i = 0; i < 3; i++) {
        float rayDirComponent = (i == 0) ? rayDir.x : (i == 1) ? rayDir.y : rayDir.z;
        float rayOriginComponent = (i == 0) ? rayOrigin.x : (i == 1) ? rayOrigin.y : rayOrigin.z;
        float boxMinComponent = (i == 0) ? boxMin.x : (i == 1) ? boxMin.y : boxMin.z;
        float boxMaxComponent = (i == 0) ? boxMax.x : (i == 1) ? boxMax.y : boxMax.z;
        
        if (std::abs(rayDirComponent) < 1e-6f) {
            // Ray is parallel to slab
            if (rayOriginComponent < boxMinComponent || rayOriginComponent > boxMaxComponent) {
                return false;
            }
        } else {
            float t1 = (boxMinComponent - rayOriginComponent) / rayDirComponent;
            float t2 = (boxMaxComponent - rayOriginComponent) / rayDirComponent;
            
            if (t1 > t2) std::swap(t1, t2);
            
            if (t1 > tmin) {
                tmin = t1;
                // Set normal based on which face we hit
                hitNormal = Vector3Game(0, 0, 0);
                if (i == 0) hitNormal.x = (rayDirComponent > 0) ? -1.0f : 1.0f;
                else if (i == 1) hitNormal.y = (rayDirComponent > 0) ? -1.0f : 1.0f;
                else hitNormal.z = (rayDirComponent > 0) ? -1.0f : 1.0f;
            }
            
            if (t2 < tmax) tmax = t2;
            
            if (tmin > tmax) return false;
        }
    }
    
    if (tmin > 0) {
        distance = tmin;
        normal = hitNormal;
        return true;
    }
    
    return false;
}

void Workspace::SynchronizeCameras() {
    if (synchronizingCameras || !camera) return;
    
    // Only sync if the current script context allows it
    if (!ShouldSyncCameraChanges()) return;
    
    synchronizingCameras = true;
    
    // Synchronize CurrentCamera properties to all other cameras
    for (auto& syncedCamera : syncedCameras) {
        if (!syncedCamera || !syncedCamera->Alive) continue;
        
        // Copy all camera properties from CurrentCamera to synced camera
        syncedCamera->CFrameValue = camera->CFrameValue;
        syncedCamera->Focus = camera->Focus;
        syncedCamera->FieldOfView = camera->FieldOfView;
        syncedCamera->CameraType = camera->CameraType;
        syncedCamera->HeadLocked = camera->HeadLocked;
        syncedCamera->HeadScale = camera->HeadScale;
        syncedCamera->VRTiltAndRollEnabled = camera->VRTiltAndRollEnabled;
        
        // Update legacy properties
        syncedCamera->Position = camera->Position;
        syncedCamera->Target = camera->Target;
        
        // Update derived FOV values
        syncedCamera->UpdateDerivedFOV();
    }
    
    synchronizingCameras = false;
}

void Workspace::RegisterCameraForSync(std::shared_ptr<CameraGame> cam) {
    if (!cam || cam->Name == "CurrentCamera") return;
    
    // Check if already registered
    auto it = std::find(syncedCameras.begin(), syncedCameras.end(), cam);
    if (it != syncedCameras.end()) return;
    
    syncedCameras.push_back(cam);
    
    // Immediately sync the new camera with CurrentCamera
    if (camera && !synchronizingCameras) {
        SynchronizeCameras();
    }
}

void Workspace::UnregisterCameraFromSync(std::shared_ptr<CameraGame> cam) {
    auto it = std::find(syncedCameras.begin(), syncedCameras.end(), cam);
    if (it != syncedCameras.end()) {
        syncedCameras.erase(it);
    }
}

// Script context management methods
void Workspace::SetCurrentScriptContext(RunContext context) {
    currentScriptContext = context;
    hasSetScriptContext = true;
}

RunContext Workspace::GetScriptContextFromLuaState(lua_State* L) const {
    // Get the BaseScript from the Lua thread data
    BaseScript* script = static_cast<BaseScript*>(lua_getthreaddata(L));
    if (!script) {
        // If no script data, return default context
        return RunContext::Plugin;
    }
    
    // Check the script's source for context directives
    std::string source = script->Source;
    std::string firstLines = source.substr(0, std::min(source.size(), size_t(200)));
    
    if (firstLines.find("--&clientscript") != std::string::npos) {
        return RunContext::Client;
    } else if (firstLines.find("--&serverscript") != std::string::npos) {
        return RunContext::Server;
    }
    
    // Default to Plugin context if no directive found
    return RunContext::Plugin;
}

bool Workspace::ShouldShowCurrentCamera() const {
    if (!hasSetScriptContext) return true; // Default behavior - show CurrentCamera
    
    switch (currentScriptContext) {
        case RunContext::Client:
            return false; // Client scripts hide CurrentCamera from explorer (but can access via code)
        case RunContext::Server:
            return false; // Server scripts hide CurrentCamera from explorer (and cannot access via code)
        case RunContext::Plugin:
            return true;  // Plugin scripts can see CurrentCamera
        default:
            return true;
    }
}

bool Workspace::ShouldSyncCameraChanges() const {
    if (!hasSetScriptContext) return true; // Default behavior - sync changes
    
    switch (currentScriptContext) {
        case RunContext::Client:
            return true;  // Client scripts sync camera changes in real-time
        case RunContext::Server:
            return false; // Server scripts don't sync camera changes in real-time
        case RunContext::Plugin:
            return true;  // Plugin scripts sync camera changes
        default:
            return true;
    }
}

bool Workspace::ShouldAllowCameraModification() const {
    if (!hasSetScriptContext) return true; // Default behavior - allow modifications
    
    switch (currentScriptContext) {
        case RunContext::Client:
            return true;  // Client scripts can modify camera properties
        case RunContext::Server:
            return false; // Server scripts cannot modify camera properties (read-only)
        case RunContext::Plugin:
            return true;  // Plugin scripts can modify camera properties
        default:
            return true;
    }
}

static Instance::Registrar _reg_ws("Workspace", []{
    return std::make_shared<Workspace>("Workspace");
});
