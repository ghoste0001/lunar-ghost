#pragma once

#include "bootstrap/services/Service.h"
#include <string>
#include <vector>
#include <sstream>
#include <cstdarg>

void LogToFile(const std::vector<std::string>& messages);
void LogToConsole(const std::vector<std::string>& messages);
void LogBoth(const std::vector<std::string>& messages);
void RaylibLog(int level, const char* fmt, va_list args);


template<typename... Args>
void LogBoth(Args... args)
{
    std::stringstream ss;
    (ss << ... << args);

    LogBoth({ ss.str() });
}


// Forward declaration for lua_State
struct lua_State;

class LogService : public Service {
private:
    std::string message;
public:
    // Constructor
    LogService();

    bool LuaGet(lua_State* L, const char* k) const override;
    bool LuaSet(lua_State* L, const char* k, int idx) override;
};