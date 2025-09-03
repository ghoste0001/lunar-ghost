#include "Folder.h"
#include "core/logging/Logging.h"

FolderInstance::FolderInstance(std::string name) : Instance(name, InstanceClass::Folder) {
    LOGI("Folder created '%s'", name.c_str());
}

FolderInstance::~FolderInstance() {
}

bool FolderInstance::LuaGet(lua_State* L, const char* key) const {
    return Instance::LuaGet(L, key);
}

bool FolderInstance::LuaSet(lua_State* L, const char* key, int valueIndex) {
    return Instance::LuaSet(L, key, valueIndex);
}

static Instance::Registrar reg_Folder("Folder", []() {
    return std::make_shared<FolderInstance>();
});
