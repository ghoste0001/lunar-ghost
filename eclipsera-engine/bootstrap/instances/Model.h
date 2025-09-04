#pragma once
#include "bootstrap/Instance.h"
#include "core/datatypes/Vector3Game.h"
#include "core/datatypes/CFrame.h"

// Forward declare Lua
struct lua_State;

struct BasePart;

struct ModelInstance : Instance {
    // Properties
    std::weak_ptr<BasePart> PrimaryPart;
    CFrame WorldPivot;
    double Scale{1.0};
    
    // Constructor
    ModelInstance(std::string name = "Model");
    ~ModelInstance() override;

    // Methods
    std::pair<CFrame, ::Vector3> GetBoundingBox() const;
    ::Vector3 GetExtentsSize() const;
    void MoveTo(const ::Vector3& position);
    void TranslateBy(const ::Vector3& delta);
    void ScaleTo(double newScaleFactor);
    double GetScale() const;
    CFrame GetPivot() const;
    void PivotTo(const CFrame& targetCFrame);

    // Lua property hooks
    bool LuaGet(lua_State* L, const char* key) const override;
    bool LuaSet(lua_State* L, const char* key, int valueIndex) override;

    // Clone support
    void RemapReferences(const CloneMap& cloneMap) override;

private:
    bool hasExplicitWorldPivot{false};
    CFrame calculateCenterOfBoundingBox() const;
    void updateChildrenTransforms(const CFrame& oldPivot, const CFrame& newPivot, double scaleRatio = 1.0);
};
