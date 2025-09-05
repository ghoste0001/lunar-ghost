#pragma once
#include "bootstrap/Instance.h"

// Forward declare Lua
struct lua_State;

struct FolderInstance : Instance {
    // Constructor
    FolderInstance(std::string name = "Folder");
    ~FolderInstance() override;

    // Lua property hooks
    bool LuaGet(lua_State* L, const char* key) const override;
    bool LuaSet(lua_State* L, const char* key, int valueIndex) override;
};
