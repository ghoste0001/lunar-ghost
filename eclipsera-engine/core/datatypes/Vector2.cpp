#include "Vector2.h"
#include <cstring> // For strcmp
using namespace lb;

// --- Constructor ---
static int v2_new(lua_State* L){
    float x=(float)luaL_optnumber(L,1,0), y=(float)luaL_optnumber(L,2,0);
    push(L, Vector2Game{x,y});
    return 1;
}

// --- Metamethods ---
static int v2_tostring(lua_State* L){
    auto* v = check<Vector2Game>(L,1);
    lua_pushfstring(L,"%f, %f",v->x,v->y); return 1;
}
static int v2_add(lua_State* L){
    auto* a=check<Vector2Game>(L,1); auto* b=check<Vector2Game>(L,2);
    push(L, *a + *b); return 1;
}
static int v2_sub(lua_State* L){
    auto* a=check<Vector2Game>(L,1); auto* b=check<Vector2Game>(L,2);
    push(L, *a - *b); return 1;
}
static int v2_mul(lua_State* L){ // scalar * v OR v * scalar
    if (lua_isnumber(L,1)) {
        float s=(float)lua_tonumber(L,1); auto* v=check<Vector2Game>(L,2);
        push(L, *v * s);
    } else {
        auto* v=check<Vector2Game>(L,1); float s=(float)luaL_checknumber(L,2);
        push(L, *v * s);
    }
    return 1;
}
static int v2_div(lua_State* L){ // v / scalar
    auto* v=check<Vector2Game>(L,1); float s=(float)luaL_checknumber(L,2);
    if (s == 0.0f) {
        luaL_error(L, "division by zero");
        return 0; 
    }
    push(L, *v / s); return 1;
}
static int v2_unm(lua_State* L){ // -v
    auto* v=check<Vector2Game>(L,1);
    push(L, -(*v)); return 1;
}

// --- Methods ---
static int v2_dot(lua_State* L) {
    auto* a=check<Vector2Game>(L,1); auto* b=check<Vector2Game>(L,2);
    lua_pushnumber(L, a->dot(*b));
    return 1;
}
static int v2_lerp(lua_State* L) {
    auto* a=check<Vector2Game>(L,1);
    auto* b=check<Vector2Game>(L,2);
    float alpha = (float)luaL_checknumber(L,3);
    push(L, a->lerp(*b, alpha));
    return 1;
}

// --- __index for properties and methods ---
static int v2_index(lua_State* L) {
    auto* v = check<Vector2Game>(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strcmp(key, "X") == 0) {
        lua_pushnumber(L, v->x);
        return 1;
    }
    else if (strcmp(key, "Y") == 0) {
        lua_pushnumber(L, v->y);
        return 1;
    }
    else if (strcmp(key, "Magnitude") == 0) {
        lua_pushnumber(L, v->magnitude());
        return 1;
    }
    else if (strcmp(key, "Unit") == 0) {
        push(L, v->normalized());
        return 1;
    }
    else {
        // Look for method in the methods table stored in metatable
        luaL_getmetatable(L, Traits<Vector2Game>::MetaName());
        lua_getfield(L, -1, "__methods");
        if (!lua_isnil(L, -1)) {
            lua_getfield(L, -1, key);
            if (!lua_isnil(L, -1)) {
                return 1; // Found the method, return it
            }
        }
        
        // Method not found
        luaL_error(L, "invalid member '%s' for Vector2", key);
        return 0;
    }
}

static const luaL_Reg V2_METHODS[] = {
    {"Dot", v2_dot},
    {"Lerp", v2_lerp},
    {nullptr,nullptr}
};

static const luaL_Reg V2_META[] = {
    {"__tostring", v2_tostring},
    {"__add",      v2_add},
    {"__sub",      v2_sub},
    {"__mul",      v2_mul},
    {"__div",      v2_div},
    {"__unm",      v2_unm},
    {"__index",    v2_index},
    {nullptr,nullptr}
};

lua_CFunction Traits<Vector2Game>::Ctor() { return v2_new; }
const luaL_Reg* Traits<Vector2Game>::Methods() { return V2_METHODS; }
const luaL_Reg* Traits<Vector2Game>::MetaMethods() { return V2_META; }
const luaL_Reg* Traits<Vector2Game>::Statics() { return nullptr; }
