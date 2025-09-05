#include "bootstrap/instances/Sky.h"
#include "core/logging/Logging.h"
#include <cstring>
#include "lua.h"
#include "lualib.h"
#include "luacode.h"

Sky::Sky(std::string name)
    : Instance(std::move(name), InstanceClass::Sky) {
    LOGI("Sky created '%s'", Name.c_str());
}

Sky::~Sky() = default;

bool Sky::LuaGet(lua_State* L, const char* key) const {
    if (std::strcmp(key, "SkyboxBk") == 0) {
        lua_pushstring(L, SkyboxBk.c_str());
        return true;
    }
    if (std::strcmp(key, "SkyboxDn") == 0) {
        lua_pushstring(L, SkyboxDn.c_str());
        return true;
    }
    if (std::strcmp(key, "SkyboxFt") == 0) {
        lua_pushstring(L, SkyboxFt.c_str());
        return true;
    }
    if (std::strcmp(key, "SkyboxLf") == 0) {
        lua_pushstring(L, SkyboxLf.c_str());
        return true;
    }
    if (std::strcmp(key, "SkyboxRt") == 0) {
        lua_pushstring(L, SkyboxRt.c_str());
        return true;
    }
    if (std::strcmp(key, "SkyboxUp") == 0) {
        lua_pushstring(L, SkyboxUp.c_str());
        return true;
    }
    if (std::strcmp(key, "CelestialBodiesShown") == 0) {
        lua_pushboolean(L, CelestialBodiesShown);
        return true;
    }
    if (std::strcmp(key, "StarCount") == 0) {
        lua_pushnumber(L, StarCount);
        return true;
    }
    if (std::strcmp(key, "SunAngularSize") == 0) {
        lua_pushnumber(L, SunAngularSize);
        return true;
    }
    if (std::strcmp(key, "SunTextureId") == 0) {
        lua_pushstring(L, SunTextureId.c_str());
        return true;
    }
    if (std::strcmp(key, "MoonAngularSize") == 0) {
        lua_pushnumber(L, MoonAngularSize);
        return true;
    }
    if (std::strcmp(key, "MoonTextureId") == 0) {
        lua_pushstring(L, MoonTextureId.c_str());
        return true;
    }
    return false;
}

bool Sky::LuaSet(lua_State* L, const char* key, int valueIndex) {
    if (std::strcmp(key, "SkyboxBk") == 0) {
        const char* value = luaL_checkstring(L, valueIndex);
        SkyboxBk = value ? value : "";
        return true;
    }
    if (std::strcmp(key, "SkyboxDn") == 0) {
        const char* value = luaL_checkstring(L, valueIndex);
        SkyboxDn = value ? value : "";
        return true;
    }
    if (std::strcmp(key, "SkyboxFt") == 0) {
        const char* value = luaL_checkstring(L, valueIndex);
        SkyboxFt = value ? value : "";
        return true;
    }
    if (std::strcmp(key, "SkyboxLf") == 0) {
        const char* value = luaL_checkstring(L, valueIndex);
        SkyboxLf = value ? value : "";
        return true;
    }
    if (std::strcmp(key, "SkyboxRt") == 0) {
        const char* value = luaL_checkstring(L, valueIndex);
        SkyboxRt = value ? value : "";
        return true;
    }
    if (std::strcmp(key, "SkyboxUp") == 0) {
        const char* value = luaL_checkstring(L, valueIndex);
        SkyboxUp = value ? value : "";
        return true;
    }
    if (std::strcmp(key, "CelestialBodiesShown") == 0) {
        CelestialBodiesShown = lua_toboolean(L, valueIndex);
        return true;
    }
    if (std::strcmp(key, "StarCount") == 0) {
        StarCount = (float)luaL_checknumber(L, valueIndex);
        return true;
    }
    if (std::strcmp(key, "SunAngularSize") == 0) {
        SunAngularSize = (float)luaL_checknumber(L, valueIndex);
        return true;
    }
    if (std::strcmp(key, "SunTextureId") == 0) {
        const char* value = luaL_checkstring(L, valueIndex);
        SunTextureId = value ? value : "";
        return true;
    }
    if (std::strcmp(key, "MoonAngularSize") == 0) {
        MoonAngularSize = (float)luaL_checknumber(L, valueIndex);
        return true;
    }
    if (std::strcmp(key, "MoonTextureId") == 0) {
        const char* value = luaL_checkstring(L, valueIndex);
        MoonTextureId = value ? value : "";
        return true;
    }
    return false;
}

static Instance::Registrar _reg_sky("Sky", [] {
    return std::make_shared<Sky>("Sky");
});
