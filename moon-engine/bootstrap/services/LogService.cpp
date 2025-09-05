#include "bootstrap/services/LogService.h"
#include "bootstrap/Game.h"
#include "bootstrap/Instance.h"
#include "lua.h"
#include "lualib.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <algorithm>

// ngl 99% of this was written by ai...
// dont judge me pls, this shit is hard and ive never used c++ b4.

// --- log rotation shiiii ---
static std::string G_LOG_FILE_PATH = "";
void ManageLogRotation()
{
    const int MAX_LOG_FILES = 5;
    const std::string logDirectory = "Logs";

    if (!std::filesystem::exists(logDirectory)) {
        return; // Directory doesn't exist, nothing to do.
    }

    // 1. Collect all .log files from the directory.
    std::vector<std::filesystem::path> logFiles;
    for (const auto& entry : std::filesystem::directory_iterator(logDirectory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".log")
        {
            logFiles.push_back(entry.path());
        }
    }

    if (logFiles.size() < MAX_LOG_FILES) {
        return;
    }

    std::sort(logFiles.begin(), logFiles.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) {
        return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
        });

    while (logFiles.size() >= MAX_LOG_FILES)
    {
        try {
            std::cout << "Log rotation: Deleting oldest log file -> " << logFiles.front().string() << std::endl;
            std::filesystem::remove(logFiles.front());
            logFiles.erase(logFiles.begin()); // Remove the deleted file from our list
        }
        catch (const std::filesystem::filesystem_error& err) {
            std::cerr << "Log rotation error: " << err.what() << std::endl;
        }
    }
}

void EnsureLogFileIsReady()
{
    if (!G_LOG_FILE_PATH.empty()) {
        return;
    }
    ManageLogRotation();

    std::filesystem::create_directory("Logs");

    std::time_t now = std::time(nullptr);
    std::tm local_time;
    localtime_s(&local_time, &now); // Use localtime_s for safety on Windows

    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &local_time);

    G_LOG_FILE_PATH = "Logs/" + std::string(buffer) + ".log";
}

// --- core funcs ---

void LogToFile(const std::vector<std::string>& messages) {
    EnsureLogFileIsReady();
    std::stringstream ss;
    for (size_t i = 0; i < messages.size(); ++i) {
        ss << messages[i] << (i < messages.size() - 1 ? "\t" : "");
    }
    std::ofstream logFile(G_LOG_FILE_PATH, std::ios::app);
    if (logFile.is_open()) {
        logFile << ss.str() << std::endl;
    }
}

void LogToConsole(const std::vector<std::string>& messages) {
    std::stringstream ss;
    for (size_t i = 0; i < messages.size(); ++i) {
        ss << messages[i] << (i < messages.size() - 1 ? "\t" : "");
    }
    printf("%s\n", ss.str().c_str());
}

// log both to console and the log file.
void LogBoth(const std::vector<std::string>& messages) {
    LogToConsole(messages);
    LogToFile(messages);
}


// --- the callback function for raylib logging. ---
void RaylibLog(int level, const char* fmt, va_list args) {
    char message_buffer[2048];
    vsnprintf(message_buffer, sizeof(message_buffer), fmt, args);
    // The adapter now calls LogBoth to send Raylib's logs to both places.
    LogBoth({ std::string(message_buffer) });
}


// --- luau ---
static int lua_LogService_Log(lua_State* L) {
    std::vector<std::string> messages;
    int nargs = lua_gettop(L);
    int starting_index = (nargs >= 1 && lua_isuserdata(L, 1)) ? 2 : 1;
    for (int i = starting_index; i <= nargs; i++) {
        luaL_tolstring(L, i, NULL);
        const char* str = lua_tostring(L, -1);
        if (str) { messages.push_back(str); }
        lua_pop(L, 1);
    }
    LogToConsole(messages); // Lua bridge calls LogBoth
    return 0;
}
static int lua_LogService_LogToFile(lua_State* L) {
    std::vector<std::string> messages;
    int nargs = lua_gettop(L);
    int starting_index = (nargs >= 1 && lua_isuserdata(L, 1)) ? 2 : 1;
    for (int i = starting_index; i <= nargs; i++) {
        luaL_tolstring(L, i, NULL);
        const char* str = lua_tostring(L, -1);
        if (str) { messages.push_back(str); }
        lua_pop(L, 1);
    }
    LogToFile(messages);
    return 0;
}
static int lua_LogService_LogBoth(lua_State* L) {
    std::vector<std::string> messages;
    int nargs = lua_gettop(L);
    int starting_index = (nargs >= 1 && lua_isuserdata(L, 1)) ? 2 : 1;
    for (int i = starting_index; i <= nargs; i++) {
        luaL_tolstring(L, i, NULL);
        const char* str = lua_tostring(L, -1);
        if (str) { messages.push_back(str); }
        lua_pop(L, 1);
    }
    LogBoth(messages);
    return 0;
}

LogService::LogService() : Service("LogService", InstanceClass::LogService) {}
bool LogService::LuaGet(lua_State* L, const char* k) const {
    if (strcmp(k, "Log") == 0) {
        lua_pushcfunction(L, lua_LogService_Log, "Log");
        return true;
    }
    if (strcmp(k, "LogToFile") == 0) {
        lua_pushcfunction(L, lua_LogService_LogToFile, "LogToFile");
        return true;
    }
    if (strcmp(k, "LogBoth") == 0) {
        lua_pushcfunction(L, lua_LogService_LogBoth, "LogBoth");
        return true;
    }
    // ... other properties
    return false;
}

//bool LogService::LuaGet(lua_State* L, const char* k) const
//{
//    if (strcmp(k, "Version") == 0) { lua_pushstring(L, "1.0.0"); return true; }
//    if (strcmp(k, "Message") == 0) { lua_pushstring(L, this->message.c_str()); return true; }
//    if (strcmp(k, "LogToFile") == 0) { lua_pushcfunction(L, lua_LogService_LogToFile, "LogToFile"); return true; }
//    if (strcmp(k, "Log") == 0) { lua_pushcfunction(L, lua_LogService_Log, "Log"); return true; }
//    return false;
//}

bool LogService::LuaSet(lua_State* L, const char* k, int idx)
{
    if (strcmp(k, "Message") == 0)
    {
        size_t len;
        const char* str = lua_tolstring(L, idx, &len);
        if (str) { this->message = std::string(str, len); }
        return true;
    }
    return false;
}

static Instance::Registrar s_regLog("LogService", []() {
    return std::make_shared<LogService>();
    });