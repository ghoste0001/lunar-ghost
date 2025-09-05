#include "bootstrap/instances/CameraGame.h"
#include "bootstrap/instances/Workspace.h"
#include "bootstrap/Game.h"
#include "bootstrap/ScriptingAPI.h"
#include "core/logging/Logging.h"
#include "core/datatypes/LuaDatatypes.h"
#include <memory>
#include <raylib.h>
#include <raymath.h>

extern std::shared_ptr<Game> g_game;

CameraGame::CameraGame(std::string name) : Instance(std::move(name), InstanceClass::Camera) {
    LOGI("CameraGame created");
}
CameraGame::~CameraGame() = default;

bool CameraGame::LuaGet(lua_State* L, const char* k) const {
    // Handle CameraGame-specific properties
    if (strcmp(k, "CFrame") == 0) {
        lb::push(L, CFrameValue);
        return true;
    }
    if (strcmp(k, "Focus") == 0) {
        lb::push(L, Focus);
        return true;
    }
    if (strcmp(k, "Position") == 0) {
        lb::push(L, Vector3Game::fromRay(Position));
        return true;
    }
    if (strcmp(k, "Target") == 0) {
        lb::push(L, Vector3Game::fromRay(Target));
        return true;
    }
    if (strcmp(k, "FieldOfView") == 0) {
        lua_pushnumber(L, FieldOfView);
        return true;
    }
    if (strcmp(k, "CameraType") == 0) {
        lua_pushinteger(L, static_cast<int>(CameraType));
        return true;
    }
    if (strcmp(k, "HeadLocked") == 0) {
        lua_pushboolean(L, HeadLocked);
        return true;
    }
    if (strcmp(k, "HeadScale") == 0) {
        lua_pushnumber(L, HeadScale);
        return true;
    }
    if (strcmp(k, "VRTiltAndRollEnabled") == 0) {
        lua_pushboolean(L, VRTiltAndRollEnabled);
        return true;
    }
    if (strcmp(k, "CurrentCamera") == 0) {
        // CurrentCamera property on Camera instances - context-based access
        if (g_game && g_game->workspace) {
            RunContext scriptContext = g_game->workspace->GetScriptContextFromLuaState(L);
            
            switch (scriptContext) {
                case RunContext::Client:
                    // Client scripts can access CurrentCamera property
                    if (g_game->workspace->camera) {
                        Lua_PushInstance(L, std::static_pointer_cast<Instance>(g_game->workspace->camera));
                    } else {
                        lua_pushnil(L);
                    }
                    break;
                case RunContext::Server:
                    // Server scripts cannot access CurrentCamera property
                    lua_pushnil(L);
                    break;
                case RunContext::Plugin:
                default:
                    // Plugin scripts and default behavior can access CurrentCamera
                    if (g_game->workspace->camera) {
                        Lua_PushInstance(L, std::static_pointer_cast<Instance>(g_game->workspace->camera));
                    } else {
                        lua_pushnil(L);
                    }
                    break;
            }
        } else {
            lua_pushnil(L);
        }
        return true;
    }
    
    // Fall back to parent class
    return Instance::LuaGet(L, k);
}

bool CameraGame::LuaSet(lua_State* L, const char* k, int idx) {
    // Check if camera modifications are allowed based on script context
    if (g_game && g_game->workspace && Name != "CurrentCamera") {
        // Get script context from the current Lua state
        RunContext scriptContext = g_game->workspace->GetScriptContextFromLuaState(L);
        if (scriptContext == RunContext::Server) {
            // Server scripts cannot modify Camera properties (read-only)
            return false; // Silently ignore the property change
        }
    }
    
    // Handle CameraGame-specific properties
    if (strcmp(k, "CFrame") == 0) {
        if (lb::luaL_testudata(L, idx, lb::Traits<CFrame>::MetaName())) {
            const auto* cf = lb::check<CFrame>(L, idx);
            CFrameValue = *cf;
            // Update legacy Position and Target for compatibility
            Position = cf->p.toRay();
            Target = (cf->p + cf->lookVector()).toRay();
            NotifyPropertyChanged();
            return true;
        }
    }
    if (strcmp(k, "Focus") == 0) {
        if (lb::luaL_testudata(L, idx, lb::Traits<CFrame>::MetaName())) {
            const auto* cf = lb::check<CFrame>(L, idx);
            Focus = *cf;
            NotifyPropertyChanged();
            return true;
        }
    }
    if (strcmp(k, "Position") == 0) {
        if (lb::luaL_testudata(L, idx, lb::Traits<Vector3Game>::MetaName())) {
            const auto* pos = lb::check<Vector3Game>(L, idx);
            Position = pos->toRay();
            // Update CFrame position while preserving rotation
            CFrameValue.p = *pos;
            NotifyPropertyChanged();
            return true;
        }
    }
    if (strcmp(k, "Target") == 0) {
        if (lb::luaL_testudata(L, idx, lb::Traits<Vector3Game>::MetaName())) {
            const auto* target = lb::check<Vector3Game>(L, idx);
            Target = target->toRay();
            // Update CFrame to look at target
            CFrameValue = CFrame::lookAt(CFrameValue.p, *target);
            NotifyPropertyChanged();
            return true;
        }
    }
    if (strcmp(k, "FieldOfView") == 0) {
        if (lua_isnumber(L, idx)) {
            FieldOfView = static_cast<float>(lua_tonumber(L, idx));
            UpdateDerivedFOV();
            NotifyPropertyChanged();
            return true;
        }
    }
    if (strcmp(k, "CameraType") == 0) {
        if (lua_isnumber(L, idx)) {
            CameraType = static_cast<::CameraType>(static_cast<int>(lua_tonumber(L, idx)));
            NotifyPropertyChanged();
            return true;
        }
    }
    if (strcmp(k, "HeadLocked") == 0) {
        if (lua_isboolean(L, idx)) {
            HeadLocked = lua_toboolean(L, idx);
            NotifyPropertyChanged();
            return true;
        }
    }
    if (strcmp(k, "HeadScale") == 0) {
        if (lua_isnumber(L, idx)) {
            HeadScale = static_cast<float>(lua_tonumber(L, idx));
            NotifyPropertyChanged();
            return true;
        }
    }
    if (strcmp(k, "VRTiltAndRollEnabled") == 0) {
        if (lua_isboolean(L, idx)) {
            VRTiltAndRollEnabled = lua_toboolean(L, idx);
            NotifyPropertyChanged();
            return true;
        }
    }
    
    // Fall back to parent class
    return Instance::LuaSet(L, k, idx);
}

CFrame CameraGame::GetRenderCFrame() const {
    return CFrameValue;
}

float CameraGame::GetRoll() const {
    return rollAngle;
}

void CameraGame::SetRoll(float rollAngle) {
    this->rollAngle = rollAngle;
}

void CameraGame::UpdateViewportSize(float width, float height) {
    ViewportSize = Vector2{width, height};
    UpdateDerivedFOV();
}

void CameraGame::UpdateDerivedFOV() {
    // Calculate diagonal and max axis FOV based on vertical FOV
    float aspectRatio = ViewportSize.x / ViewportSize.y;
    
    switch (FieldOfViewMode) {
        case ::FieldOfViewMode::Vertical:
            DiagonalFieldOfView = FieldOfView * sqrt(1.0f + aspectRatio * aspectRatio);
            MaxAxisFieldOfView = aspectRatio > 1.0f ? FieldOfView * aspectRatio : FieldOfView;
            break;
        case ::FieldOfViewMode::Diagonal:
            DiagonalFieldOfView = FieldOfView;
            FieldOfView = DiagonalFieldOfView / sqrt(1.0f + aspectRatio * aspectRatio);
            MaxAxisFieldOfView = aspectRatio > 1.0f ? FieldOfView * aspectRatio : FieldOfView;
            break;
        case ::FieldOfViewMode::MaxAxis:
            MaxAxisFieldOfView = FieldOfView;
            FieldOfView = aspectRatio > 1.0f ? MaxAxisFieldOfView / aspectRatio : MaxAxisFieldOfView;
            DiagonalFieldOfView = FieldOfView * sqrt(1.0f + aspectRatio * aspectRatio);
            break;
    }
}

// Screen/World conversion methods
RayGame CameraGame::ScreenPointToRay(float x, float y, float depth) const {
    // Convert screen coordinates to normalized device coordinates (-1 to 1)
    float ndcX = (2.0f * x / ViewportSize.x) - 1.0f;
    float ndcY = 1.0f - (2.0f * y / ViewportSize.y); // Flip Y axis
    
    // Create raylib camera for calculations
    Camera3D camera = {};
    camera.position = CFrameValue.p.toRay();
    camera.target = (CFrameValue.p + CFrameValue.lookVector()).toRay();
    camera.up = CFrameValue.upVector().toRay();
    camera.fovy = FieldOfView;
    camera.projection = CAMERA_PERSPECTIVE;
    
    // Use raylib's screen to world ray function
    Vector2 screenPos = {x, y};
    Ray raylibRay = GetScreenToWorldRayEx(screenPos, camera, (int)ViewportSize.x, (int)ViewportSize.y);
    
    // Convert to our RayGame format
    return RayGame::fromRaylib(raylibRay);
}

RayGame CameraGame::ViewportPointToRay(float x, float y, float depth) const {
    // Convert viewport coordinates (0-1) to screen coordinates
    float screenX = x * ViewportSize.x;
    float screenY = y * ViewportSize.y;
    return ScreenPointToRay(screenX, screenY, depth);
}

Vector2 CameraGame::WorldToScreenPoint(const Vector3Game& worldPoint) const {
    // Create raylib camera for calculations
    Camera3D camera = {};
    camera.position = CFrameValue.p.toRay();
    camera.target = (CFrameValue.p + CFrameValue.lookVector()).toRay();
    camera.up = CFrameValue.upVector().toRay();
    camera.fovy = FieldOfView;
    camera.projection = CAMERA_PERSPECTIVE;
    
    // Use raylib's world to screen function
    Vector2 screenPos = GetWorldToScreenEx(worldPoint.toRay(), camera, (int)ViewportSize.x, (int)ViewportSize.y);
    return screenPos;
}

Vector2 CameraGame::WorldToViewportPoint(const Vector3Game& worldPoint) const {
    Vector2 screenPos = WorldToScreenPoint(worldPoint);
    // Convert screen coordinates to viewport coordinates (0-1)
    return {screenPos.x / ViewportSize.x, screenPos.y / ViewportSize.y};
}

void CameraGame::ZoomToExtents(const std::vector<std::shared_ptr<Instance>>& parts) {
    if (parts.empty()) return;
    
    // Calculate bounding box of all parts
    Vector3Game minBounds(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3Game maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    
    for (const auto& part : parts) {
        // For now, assume parts have a Position property
        // In a full implementation, you'd get the actual bounding box
        Vector3Game pos(0, 0, 0); // Default position
        
        // Try to get position from the part (simplified)
        // This would need proper implementation based on part types
        minBounds.x = std::min(minBounds.x, pos.x - 1.0f);
        minBounds.y = std::min(minBounds.y, pos.y - 1.0f);
        minBounds.z = std::min(minBounds.z, pos.z - 1.0f);
        maxBounds.x = std::max(maxBounds.x, pos.x + 1.0f);
        maxBounds.y = std::max(maxBounds.y, pos.y + 1.0f);
        maxBounds.z = std::max(maxBounds.z, pos.z + 1.0f);
    }
    
    // Calculate center and size of bounding box
    Vector3Game center = (minBounds + maxBounds) * 0.5f;
    Vector3Game size = maxBounds - minBounds;
    float maxSize = std::max({size.x, size.y, size.z});
    
    // Calculate distance needed to fit the bounding box
    float distance = maxSize / (2.0f * tan(FieldOfView * DEG2RAD * 0.5f));
    
    // Position camera to look at the center from appropriate distance
    Vector3Game cameraPos = center + CFrameValue.lookVector() * (-distance * 1.5f);
    CFrameValue = CFrame::lookAt(cameraPos, center);
    
    // Update legacy properties
    Position = cameraPos.toRay();
    Target = center.toRay();
}

std::vector<std::shared_ptr<Instance>> CameraGame::GetPartsObscuringTarget(const Vector3Game& target, const std::vector<std::shared_ptr<Instance>>& parts) const {
    std::vector<std::shared_ptr<Instance>> obscuringParts;
    
    // Create ray from camera to target
    Vector3Game rayOrigin = CFrameValue.p;
    Vector3Game rayDirection = (target - rayOrigin).normalized();
    float targetDistance = (target - rayOrigin).magnitude();
    
    for (const auto& part : parts) {
        // For now, simplified collision detection
        // In a full implementation, you'd use proper ray-mesh intersection
        Vector3Game partPos(0, 0, 0); // Default position
        float partRadius = 1.0f; // Default radius
        
        // Check if ray intersects with part's bounding sphere
        Vector3Game toSphere = partPos - rayOrigin;
        float projectionLength = toSphere.dot(rayDirection);
        
        if (projectionLength > 0 && projectionLength < targetDistance) {
            Vector3Game closestPoint = rayOrigin + rayDirection * projectionLength;
            float distanceToRay = (partPos - closestPoint).magnitude();
            
            if (distanceToRay <= partRadius) {
                obscuringParts.push_back(part);
            }
        }
    }
    
    return obscuringParts;
}

void CameraGame::NotifyPropertyChanged() {
    // Only trigger synchronization if this is the CurrentCamera or a regular Camera
    if (!g_game || !g_game->workspace) return;
    
    // If this is the CurrentCamera, sync to other cameras
    if (Name == "CurrentCamera") {
        g_game->workspace->SynchronizeCameras();
    }
    // If this is a regular Camera, sync its properties to CurrentCamera
    else {
        auto currentCamera = g_game->workspace->camera;
        if (currentCamera && !g_game->workspace->IsSynchronizingCameras()) {
            // Check if we should sync camera changes based on script context
            if (g_game->workspace->ShouldSyncCameraChanges()) {
                // Copy properties from this camera to CurrentCamera (real-time sync for client scripts)
                currentCamera->CFrameValue = CFrameValue;
                currentCamera->Focus = Focus;
                currentCamera->FieldOfView = FieldOfView;
                currentCamera->CameraType = CameraType;
                currentCamera->HeadLocked = HeadLocked;
                currentCamera->HeadScale = HeadScale;
                currentCamera->VRTiltAndRollEnabled = VRTiltAndRollEnabled;
                
                // Update legacy properties
                currentCamera->Position = Position;
                currentCamera->Target = Target;
                
                // Update derived FOV values
                currentCamera->UpdateDerivedFOV();
                
                // Now sync CurrentCamera to all other cameras
                g_game->workspace->SynchronizeCameras();
            }
            // For server scripts, we still copy properties once but don't sync continuously
            else {
                static bool hasInitialSync = false;
                if (!hasInitialSync) {
                    // One-time sync for server scripts
                    currentCamera->CFrameValue = CFrameValue;
                    currentCamera->Focus = Focus;
                    currentCamera->FieldOfView = FieldOfView;
                    currentCamera->CameraType = CameraType;
                    currentCamera->HeadLocked = HeadLocked;
                    currentCamera->HeadScale = HeadScale;
                    currentCamera->VRTiltAndRollEnabled = VRTiltAndRollEnabled;
                    
                    // Update legacy properties
                    currentCamera->Position = Position;
                    currentCamera->Target = Target;
                    
                    // Update derived FOV values
                    currentCamera->UpdateDerivedFOV();
                    
                    hasInitialSync = true;
                }
            }
        }
    }
}

static Instance::Registrar _reg_cam("Camera", []{ return std::make_shared<CameraGame>("Camera"); });
