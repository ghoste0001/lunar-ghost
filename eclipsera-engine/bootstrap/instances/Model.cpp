#include "bootstrap/instances/Model.h"
#include "bootstrap/instances/BasePart.h"
#include "bootstrap/ScriptingAPI.h"
#include "core/logging/Logging.h"
#include "core/datatypes/LuaDatatypes.h"
#include <cstring>
#include <algorithm>
#include <limits>

ModelInstance::ModelInstance(std::string name)
    : Instance(std::move(name), InstanceClass::Model), WorldPivot{} {
    LOGI("Model created '%s'", Name.c_str());
}

ModelInstance::~ModelInstance() = default;

std::pair<CFrame, ::Vector3> ModelInstance::GetBoundingBox() const {
    std::vector<std::shared_ptr<BasePart>> parts;
    
    // Collect all BasePart descendants
    for (const auto& child : GetDescendants()) {
        if (auto part = std::dynamic_pointer_cast<BasePart>(child)) {
            parts.push_back(part);
        }
    }
    
    if (parts.empty()) {
        return {CFrame{}, ::Vector3{0, 0, 0}};
    }
    
    // Calculate bounding box using Vector3Game
    Vector3Game minBounds{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    Vector3Game maxBounds{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    
    for (const auto& part : parts) {
        Vector3Game halfSize = Vector3Game::fromRay(part->Size) * 0.5f;
        Vector3Game pos = part->CF.p;
        
        // For simplicity, we'll use axis-aligned bounding box
        // In a full implementation, you'd need to consider rotation
        minBounds.x = std::min(minBounds.x, pos.x - halfSize.x);
        minBounds.y = std::min(minBounds.y, pos.y - halfSize.y);
        minBounds.z = std::min(minBounds.z, pos.z - halfSize.z);
        
        maxBounds.x = std::max(maxBounds.x, pos.x + halfSize.x);
        maxBounds.y = std::max(maxBounds.y, pos.y + halfSize.y);
        maxBounds.z = std::max(maxBounds.z, pos.z + halfSize.z);
    }
    
    Vector3Game center = (minBounds + maxBounds) * 0.5f;
    Vector3Game size = maxBounds - minBounds;
    
    CFrame orientation;
    if (auto primaryPart = PrimaryPart.lock()) {
        // Use primary part's orientation
        orientation = primaryPart->CF;
        orientation.p = center;
    } else {
        // Use world-aligned orientation
        orientation = CFrame{center};
    }
    
    return {orientation, size.toRay()};
}

::Vector3 ModelInstance::GetExtentsSize() const {
    auto [orientation, size] = GetBoundingBox();
    return size;
}

void ModelInstance::MoveTo(const ::Vector3& position) {
    CFrame currentPivot = GetPivot();
    Vector3Game positionGame = Vector3Game::fromRay(position);
    
    // Move to position, but check for collisions and move up if needed
    // For simplicity, we'll just move directly for now
    // In a full implementation, you'd need collision detection
    
    CFrame newPivot = currentPivot;
    newPivot.p = positionGame;
    PivotTo(newPivot);
}

void ModelInstance::TranslateBy(const ::Vector3& delta) {
    CFrame currentPivot = GetPivot();
    Vector3Game deltaGame = Vector3Game::fromRay(delta);
    CFrame newPivot = currentPivot;
    newPivot.p = newPivot.p + deltaGame;
    PivotTo(newPivot);
}

void ModelInstance::ScaleTo(double newScaleFactor) {
    if (newScaleFactor <= 0.0) return;
    
    double oldScale = Scale;
    double scaleRatio = newScaleFactor / oldScale;
    Scale = newScaleFactor;
    
    CFrame pivot = GetPivot();
    updateChildrenTransforms(pivot, pivot, scaleRatio);
}

double ModelInstance::GetScale() const {
    return Scale;
}

CFrame ModelInstance::GetPivot() const {
    if (auto primaryPart = PrimaryPart.lock()) {
        return primaryPart->CF;
    }
    
    if (hasExplicitWorldPivot) {
        return WorldPivot;
    }
    
    // Return center of bounding box
    return calculateCenterOfBoundingBox();
}

void ModelInstance::PivotTo(const CFrame& targetCFrame) {
    CFrame oldPivot = GetPivot();
    
    if (auto primaryPart = PrimaryPart.lock()) {
        // Move primary part
        primaryPart->CF = targetCFrame;
    } else {
        // Set world pivot
        WorldPivot = targetCFrame;
        hasExplicitWorldPivot = true;
    }
    
    updateChildrenTransforms(oldPivot, targetCFrame);
}

bool ModelInstance::LuaGet(lua_State* L, const char* key) const {
    if (std::strcmp(key, "PrimaryPart") == 0) {
        if (auto part = PrimaryPart.lock()) {
            Lua_PushInstance(L, part);
        } else {
            lua_pushnil(L);
        }
        return true;
    }
    
    if (std::strcmp(key, "WorldPivot") == 0) {
        lb::push(L, GetPivot());
        return true;
    }
    
    if (std::strcmp(key, "Scale") == 0) {
        lua_pushnumber(L, Scale);
        return true;
    }
    
    return false;
}

bool ModelInstance::LuaSet(lua_State* L, const char* key, int valueIndex) {
    if (std::strcmp(key, "PrimaryPart") == 0) {
        if (lua_isnil(L, valueIndex)) {
            PrimaryPart.reset();
        } else {
            auto* inst_ptr = static_cast<std::shared_ptr<Instance>*>(
                luaL_checkudata(L, valueIndex, "Librebox.Instance"));
            if (inst_ptr && *inst_ptr) {
                if (auto part = std::dynamic_pointer_cast<BasePart>(*inst_ptr)) {
                    // Check if part is a descendant of this model
                    if (part->IsDescendantOf(shared_from_this())) {
                        PrimaryPart = part;
                    } else {
                        // Temporarily set it, but it will be reset to nil during next simulation step
                        PrimaryPart = part;
                    }
                }
            }
        }
        return true;
    }
    
    if (std::strcmp(key, "WorldPivot") == 0) {
        const auto* cf = lb::check<CFrame>(L, valueIndex);
        WorldPivot = *cf;
        hasExplicitWorldPivot = true;
        return true;
    }
    
    // Scale property is not scriptable in ROBLOX
    
    return false;
}

void ModelInstance::RemapReferences(const CloneMap& cloneMap) {
    PrimaryPart = RemapWeak(cloneMap, PrimaryPart);
}

CFrame ModelInstance::calculateCenterOfBoundingBox() const {
    auto [orientation, size] = GetBoundingBox();
    return orientation;
}

void ModelInstance::updateChildrenTransforms(const CFrame& oldPivot, const CFrame& newPivot, double scaleRatio) {
    // Calculate the transformation from old pivot to new pivot
    CFrame transform = newPivot * oldPivot.inverse();
    
    // Apply transformation to all BasePart descendants
    for (const auto& child : GetDescendants()) {
        if (auto part = std::dynamic_pointer_cast<BasePart>(child)) {
            if (part.get() == PrimaryPart.lock().get()) {
                // Primary part is already positioned correctly
                continue;
            }
            
            // Transform the part's CFrame
            part->CF = transform * part->CF;
            
            // Scale the part if needed
            if (scaleRatio != 1.0) {
                Vector3Game partSize = Vector3Game::fromRay(part->Size);
                partSize = partSize * static_cast<float>(scaleRatio);
                part->Size = partSize.toRay();
                
                // Scale position relative to pivot
                Vector3Game relativePos = part->CF.p - newPivot.p;
                relativePos = relativePos * static_cast<float>(scaleRatio);
                part->CF.p = newPivot.p + relativePos;
            }
        }
    }
}

// Register the Model type
static Instance::Registrar reg_Model("Model", []() {
    return std::make_shared<ModelInstance>();
});
