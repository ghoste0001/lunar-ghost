#pragma once
#include "bootstrap/Instance.h"
#include <string>


// note, this supposed to be done in 1-3 days but i think my brain is now fucking dead and i unable to continue doing this shit
// a bit pissed cuz i out of idea

// Forward declare Lua
struct lua_State;

struct Sky : Instance {
    // Skybox texture properties (6 faces of a cube)
    std::string SkyboxBk;  // Back face
    std::string SkyboxDn;  // Down face (bottom)
    std::string SkyboxFt;  // Front face
    std::string SkyboxLf;  // Left face
    std::string SkyboxRt;  // Right face
    std::string SkyboxUp;  // Up face (top)
    
    // Celestial body properties
    bool CelestialBodiesShown{true};
    float StarCount{3000.0f};
    
    // Sun properties
    float SunAngularSize{21.0f};
    std::string SunTextureId;
    
    // Moon properties
    float MoonAngularSize{11.0f};
    std::string MoonTextureId;
    
    Sky(std::string name);
    ~Sky() override;

    bool LuaGet(lua_State* L, const char* key) const override;
    bool LuaSet(lua_State* L, const char* key, int valueIndex) override;
};

