#include "bootstrap/instances/BasePart.h"
#include "core/logging/Logging.h"
#include "core/datatypes/Enum.h"
#include <cstring>
#include <cmath>

static inline float rad2deg(float r){ return r * 57.29577951308232f; }
static inline float deg2rad(float d){ return d * 0.017453292519943295f; }

BasePart::BasePart(std::string name, InstanceClass cls)
    : Instance(std::move(name), cls), CF{} {
    LOGI("BasePart created '%s' (Transparency=%.2f, Color=%.2f,%.2f,%.2f)", 
         Name.c_str(), Transparency, Color.r, Color.g, Color.b);
}

BasePart::~BasePart() = default;

bool BasePart::LuaGet(lua_State* L, const char* key) const {
    if (std::strcmp(key, "CFrame") == 0) {
        lb::push(L, CF);
        return true;
    }
    if (std::strcmp(key, "Position") == 0) {
        lb::push(L, CF.p);
        return true;
    }
    if (std::strcmp(key, "Orientation") == 0) {
        float rx, ry, rz;
        CF.toEulerAnglesXYZ(rx, ry, rz);
        lb::push(L, Vector3Game{ rad2deg(rx), rad2deg(ry), rad2deg(rz) });
        return true;
    }
    if (std::strcmp(key, "Size") == 0) {
        lb::push(L, Vector3Game::fromRay(Size));
        return true;
    }
    if (std::strcmp(key, "Transparency") == 0) {
        lua_pushnumber(L, Transparency);
        return true;
    }
    if (std::strcmp(key, "Color") == 0) {
        lb::push(L, Color3{ Color.r, Color.g, Color.b });
        return true;
    }
    if (std::strcmp(key, "Shape") == 0) {
        // Return the enum item for the current shape
        Enum* partTypeEnum = EnumRegistry::Instance().GetEnum("PartType");
        if (partTypeEnum) {
            EnumItem* item = partTypeEnum->GetItem(Shape);
            if (item) {
                Lua_PushEnumItem(L, item);
                return true;
            }
        }
        lua_pushnil(L);
        return true;
    }
    return false;
}

bool BasePart::LuaSet(lua_State* L, const char* key, int valueIndex) {
    if (std::strcmp(key, "CFrame") == 0) {
        const auto* cf = lb::check<CFrame>(L, valueIndex);
        CF = *cf;
        return true;
    }
    if (std::strcmp(key, "Position") == 0) {
        const auto* v = lb::check<Vector3Game>(L, valueIndex);
        CF.p = *v;
        return true;
    }
    if (std::strcmp(key, "Orientation") == 0) {
        const auto* vdeg = lb::check<Vector3Game>(L, valueIndex);
        CFrame rot = CFrame::fromEulerAnglesXYZ(
            deg2rad(vdeg->x), deg2rad(vdeg->y), deg2rad(vdeg->z));
        
        // Special handling for cylinders - fix default mesh orientation        
        if (Shape == 2) { // Cylinder
            // Cylinders need a +90-degree rotation around X-axis to stand upright????
            // i think this corrects the mesh orientation so Z=90 makes cylinder upright...
            // editted: WHY IT STILL NOT UPSTRAIGHT WHEN IT FUCKING 90DEG WHENEVER IT FUCKING ON X OR Z IT SUPPOSED TO BE UPSTRAIGHT BUT NOT BEING LIKE THIS JESUS FUCK
            CFrame cylinderFix = CFrame::fromEulerAnglesXYZ(deg2rad(90.0f), 0, 0);
            rot = rot * cylinderFix;
        }
        
        // replace rotation, keep translation
        for(int i=0;i<9;i++) CF.R[i] = rot.R[i];
        return true;
    }
    if (std::strcmp(key, "Size") == 0) {
        const auto* v = lb::check<Vector3Game>(L, valueIndex);
        Size = v->toRay();
        return true;
    }
    if (std::strcmp(key, "Transparency") == 0) {
        Transparency = (float)luaL_checknumber(L, valueIndex);
        return true;
    }
    if (std::strcmp(key, "Color") == 0) {
        const auto* c = lb::check<Color3>(L, valueIndex);
        Color = { c->r, c->g, c->b };
        return true;
    }
    if (std::strcmp(key, "Shape") == 0) {
        // Handle enum item input
        if (lua_istable(L, valueIndex)) {
            lua_getfield(L, valueIndex, "Value");
            if (lua_isnumber(L, -1)) {
                int newShape = (int)lua_tointeger(L, -1);
                if (newShape >= 0 && newShape <= 4) {
                    Shape = newShape;
                    ApplyShapeConstraints();
                }
            }
            lua_pop(L, 1);
            return true;
        }
        // Handle direct integer input
        else if (lua_isnumber(L, valueIndex)) {
            int newShape = (int)lua_tointeger(L, valueIndex);
            if (newShape >= 0 && newShape <= 4) { // Valid PartType range
                Shape = newShape;
                ApplyShapeConstraints();
            }
            return true;
        }
        return false;
    }
    return false;
}

void BasePart::ApplyShapeConstraints() {
    switch (Shape) {
        case 0: // Ball - all dimensions must be uh huh?? nah nvm, i just over thinking, yeah it must be equal
            {
                float maxDim = std::max({Size.x, Size.y, Size.z});
                Size.x = Size.y = Size.z = maxDim;
            }
            break;
        case 2: // Cylinder - X and Z must be equal (Y is height)
            {
                float maxXZ = std::max(Size.x, Size.z);
                Size.x = Size.z = maxXZ;
            }
            break;
        case 1: // Block
        case 3: // Wedge
        case 4: // CornerWedge -- i didn't do this because i don't know how it works
        default:
            // No constraints for these shapes
            break;
    }
}
