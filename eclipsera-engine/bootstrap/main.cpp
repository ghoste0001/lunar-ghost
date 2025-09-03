#include <raylib.h>
#include "Renderer.h"
#include "raymath.h"
#include "Game.h"

#include "bootstrap/instances/Script.h"
#include "core/logging/Logging.h"
#include "subsystems/filesystem/FileSystem.h"

#include "bootstrap/instances/InstanceTypes.h"
#include "bootstrap/services/RunService.h"
#include "bootstrap/services/Lighting.h"

#include "bootstrap/services/UserInputService.h"
#include "bootstrap/services/TweenService.h"
#include "bootstrap/gui/GuiManager.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#include "icon.h"
#include "icon_ico_file.h"
#include "place_file.h"
#include "preloaded_scripts.h"

// avoid Win32 name collisions
#define WIN32_LEAN_AND_MEAN
#define CloseWindow Win32CloseWindow
#define ShowCursor  Win32ShowCursor
#define DrawText Win32DrawText
#define DrawTextEx Win32DrawTextEx
#include <windows.h>
#undef DrawText
#undef DrawTextEx
#undef CloseWindow
#undef ShowCursor

static Camera3D g_camera{};
static std::unique_ptr<GuiManager> g_guiManager;

// yaw/pitch state
static float gYaw = 0.0f;
static float gPitch = 0.0f;
static bool gAnglesInit = false;
static int gTargetFPS = 0;
static std::vector<std::string> gPaths;
static bool gNoPlace = false;
static bool args = false;

static void PhysicsSimulation() {
    // stub
}

static void Cleanup();

static int LoaderMenu(int padding = 38, int gap = 10,
                      int headerHeight = 90, int instrToOptionsGap = 100) {
    int choice = 0;
    int selected = 0;

    const char* options[] = {
        "visual-darkhouse.lua",
        "visual-tempform.lua",
        "visual-sphere.lua",
        "part_example.lua"
    };
    const int optionCount = 4;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_DOWN)) selected = (selected + 1) % optionCount;
        if (IsKeyPressed(KEY_UP))   selected = (selected - 1 + optionCount) % optionCount;

        BeginDrawing();
        ClearBackground(BLACK);

        // header bar
        DrawRectangle(0, 0, GetScreenWidth(), headerHeight, WHITE);
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), "ECLIPSERA ENGINE Demo", {(float)padding, (float)(headerHeight/2 - 20)}, 40, 1.2f, BLACK);
        } else {
            DrawText("ECLIPSERA ENGINE Demo", padding, headerHeight/2 - 20, 40, BLACK);
        }

        // instructions
        int instrY = headerHeight + 20;
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), "WASD to move, Arrow keys rotate", {(float)padding, (float)instrY}, 20, 1.2f, GRAY);
            DrawTextEx(GuiManager::GetCustomFont(), "Use UP/DOWN to select, ENTER or (1-4) to confirm", {(float)padding, (float)(instrY + 30)}, 20, 1.2f, GRAY);
            DrawTextEx(GuiManager::GetCustomFont(), "THIS IS A MODIFICATION OF LUNAR-ENGINE NOT OFFICIAL!", {(float)padding, (float)(instrY + 60)}, 20, 1.2f, GRAY);
        } else {
            DrawText("WASD to move, Arrow keys rotate", padding, instrY, 20, GRAY);
            DrawText("Use UP/DOWN to select, ENTER or (1-4) to confirm", padding, instrY + 30, 20, GRAY);
            DrawText("THIS IS A MODIFICATION OF LUNAR-ENGINE NOT OFFICIAL!", padding, instrY + 60, 20, GRAY);
        }

        // options start after instructions + adjustable gap
        int baseY = instrY + 45 + instrToOptionsGap;
        int boxW = 400;
        int boxH = 50;
        for (int i = 0; i < optionCount; i++) {
            int y = baseY + i * (boxH + gap);
            Color bg = (i == selected) ? DARKGRAY : RAYWHITE;
            Color fg = (i == selected) ? YELLOW   : BLACK;

            DrawRectangle(padding, y, boxW, boxH, bg);
            if (GuiManager::IsCustomFontLoaded()) {
                DrawTextEx(GuiManager::GetCustomFont(), options[i], {(float)(padding + 10), (float)(y + 15)}, 20, 1.2f, fg);
            } else {
                DrawText(options[i], padding + 10, y + 15, 20, fg);
            }
        }

        int creditsY = baseY + optionCount * (boxH + gap) + 40;
        if (GuiManager::IsCustomFontLoaded()) {
            DrawTextEx(GuiManager::GetCustomFont(), "Made by LunarEngine Developers Modified by nickibreeki", 
                      {(float)padding, (float)creditsY}, 20, 1.2f, RED);
        } else {
            DrawText("Made by LunarEngine Developers Modified by nickibreeki", padding, creditsY, 20, RED);
        }

        EndDrawing();

        if (IsKeyPressed(KEY_ONE))   { choice = 1; break; }
        if (IsKeyPressed(KEY_TWO))   { choice = 2; break; }
        if (IsKeyPressed(KEY_THREE)) { choice = 3; break; }
        if (IsKeyPressed(KEY_FOUR))  { choice = 4; break; }
        if (IsKeyPressed(KEY_ENTER)) { choice = selected + 1; break; }
    }
    return choice;
}

static void Stage_ConfigInitialization() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    std::atexit(&Cleanup);
    LOGI("Loaded configuration");
}

static void LoadAndScheduleScript(const std::string& name, const std::string& path) {
    auto script = std::make_shared<Script>(name, fsys::ReadFileToString(path));
    if (g_game && g_game->workspace) {
        script->SetParent(g_game->workspace);
    }
    script->Schedule();
    LOGI("Scheduled script: %s", path.c_str());
}

static std::filesystem::path GetExecutableDirectory() {
    namespace fs = std::filesystem;
    
    #ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
    #else
    // For other platforms, you might need different implementations
    return fs::current_path();
    #endif
}

static bool Preflight_ValidateResources() {
    namespace fs = std::filesystem;
    
    // Get the directory where the executable is located
    fs::path executableDir = GetExecutableDirectory();
    
    // Check if resources folder exists relative to executable
    fs::path resourcesPath = executableDir / "resources";
    if (!fs::exists(resourcesPath)) {
        LOGE("Resources folder not found at: %s", resourcesPath.string().c_str());
        LOGE("The application cannot launch without the resources folder.");
        LOGE("Please ensure the resources folder is in the same directory as the executable.");
        LOGE("Executable directory: %s", executableDir.string().c_str());
        
        // Show error message box on Windows
        #ifdef _WIN32
        std::string errorMsg = "Resources folder not found!\n\n"
                              "The application cannot launch without the resources folder.\n"
                              "Please ensure the resources folder is in the same directory as the executable.\n\n"
                              "Executable location: " + executableDir.string() + "\n"
                              "Looking for: " + resourcesPath.string();
        MessageBoxA(NULL, errorMsg.c_str(), "Eclipsera Engine - Missing Resources", MB_OK | MB_ICONERROR);
        #endif
        
        return false;
    }
    
    // Check for critical resource files
    fs::path instanceIconsPath = resourcesPath / "eclipsera-contents" / "textures" / "core" / "instance";
    if (!fs::exists(instanceIconsPath)) {
        LOGE("Instance icons folder not found at: %s", instanceIconsPath.string().c_str());
        LOGE("Critical resource files are missing from the resources folder.");
        
        #ifdef _WIN32
        MessageBoxA(NULL, 
            "Critical resource files are missing!\n\n"
            "The instance icons folder was not found in the resources directory.\n"
            "Please ensure you have the complete resources folder.",
            "Eclipsera Engine - Incomplete Resources", 
            MB_OK | MB_ICONWARNING);
        #endif
        
        return false;
    }
    
    LOGI("Resources validation passed");
    LOGI("Resources found at: %s", resourcesPath.string().c_str());
    return true;
}

static bool Preflight_ValidatePaths() {
    if (gPaths.empty()) return true;
    namespace fs = std::filesystem;
    for (const auto& pathStr : gPaths) {
        fs::path p(pathStr);
        if (!fs::exists(p)) {
            if (p.has_extension()) {
                LOGE("Script file not found: %s", pathStr.c_str());
                return false; // exit before window/renderer
            } else {
                LOGE("Path does not exist: %s", pathStr.c_str());
            }
        }
    }
    return true;
}

static void Stage_Initialization() {
    LOGI("Stage: Initialization begin");

    InitWindow(1280, 720, "ECLIPSERA ENGINE (LunarEngine 1.0.0 modified)");

    // setup icons
    Image icon = {};
    icon.data = icon_data;
    icon.width = icon_data_width;
    icon.height = icon_data_height;
    icon.mipmaps = 1;
    icon.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    SetWindowIcon(icon);
    HICON hIcon = CreateIconFromResourceEx(
        (PBYTE)icon_ico,
        (DWORD)icon_ico_len,
        TRUE,
        0x00030000,
        0, 0,
        LR_DEFAULTCOLOR
    );

    if (hIcon) {
        HWND hwnd = GetActiveWindow();
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }


    LOGI("Raylib window initialized");

    int selected = 0;
    
    if (!args) {
        selected = LoaderMenu();
    }

    g_game = std::make_shared<Game>();
    g_game->Init();

    // Initialize GUI Manager BEFORE scheduling any scripts
    // This ensures icons are loaded before scripts start executing
    g_guiManager = std::make_unique<GuiManager>();
    g_guiManager->Initialize();
    LOGI("GUI Manager initialized (icons loaded before scripts)");

    // load script if needed
    if (selected > 0) {
        const unsigned char* data = nullptr;
        size_t len = 0;

        switch (selected) {
            case 1: data = preloaded_script_1; len = preloaded_script_1_len; break;
            case 2: data = preloaded_script_2; len = preloaded_script_2_len; break;
            case 3: data = preloaded_script_3; len = preloaded_script_3_len; break;
            case 4: data = preloaded_script_4; len = preloaded_script_4_len; break;
        }

        if (data && len) {
            auto script = std::make_shared<Script>(
                "preloaded",
                std::string(reinterpret_cast<const char*>(data), len)
            );
            if (g_game->workspace) {
                script->SetParent(g_game->workspace);
            }
            script->Schedule();
            LOGI("Scheduled preloaded script %d", selected);
        }
    }

    g_camera = {};
    g_camera.up = {0.0f, 1.0f, 0.0f};
    g_camera.fovy = 70.0f;
    g_camera.projection = CAMERA_PERSPECTIVE;
    if (g_game->workspace && g_game->workspace->camera) {
        g_camera.position = g_game->workspace->camera->Position;
        g_camera.target   = g_game->workspace->camera->Target;
    } else {
        g_camera.position = {0, 2, -5};
        g_camera.target   = {0, 2, 0};
    }

    // init yaw/pitch from initial camera
    if (!gAnglesInit) {
        Vector3 f0 = Vector3Normalize(Vector3Subtract(g_camera.target, g_camera.position));
        gYaw   = atan2f(f0.z, f0.x);
        gPitch = asinf(Clamp(f0.y, -1.0f, 1.0f));
        gAnglesInit = true;
    }

    // Load the place script
    if (!gNoPlace) {
        auto placeInitScript = std::make_shared<Script>("place", std::string(place_script, place_script_len));
        placeInitScript->Schedule();
    }

    // Path handling (supports multiple --path)
    if (!gPaths.empty()) {
        namespace fs = std::filesystem;
        for (const auto& pathStr : gPaths) {
            fs::path p(pathStr);
            if (fs::exists(p)) {
                if (fs::is_regular_file(p)) {
                    LoadAndScheduleScript(p.stem().string(), p.string()); // run regardless of extension
                } else if (fs::is_directory(p)) {
                    for (auto& entry : fs::directory_iterator(p)) {
                        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                            LoadAndScheduleScript(entry.path().stem().string(), entry.path().string());
                        }
                    }
                } else {
                    LOGE("Path is neither file nor directory: %s", pathStr.c_str());
                }
            } else {
                // Already reported during preflight
            }
        }
    }

    LOGI("Stage: Initialization end");
}

static void Stage_Run() {
    LOGI("Stage: Run loop begin");
    auto rs = std::dynamic_pointer_cast<RunService>(Service::Get("RunService"));
    auto ls = std::dynamic_pointer_cast<Lighting>(Service::Get("Lighting"));

    while (!WindowShouldClose()) {
        const double now = GetTime();
        const double dt  = GetFrameTime();

        lua_State* Lm = (g_game && g_game->luaScheduler) ? g_game->luaScheduler->GetMainState() : nullptr;
        if (rs && Lm) {
            rs->EnsureSignals();
            if (rs->PreRender && !rs->PreRender->IsClosed()) {
                lua_pushnumber(Lm, dt);
                rs->PreRender->Fire(Lm, lua_gettop(Lm), 1);
                lua_pop(Lm, 1);
            }
            if (rs->PreAnimation && !rs->PreAnimation->IsClosed()) {
                lua_pushnumber(Lm, dt);
                rs->PreAnimation->Fire(Lm, lua_gettop(Lm), 1);
                lua_pop(Lm, 1);
            }
            if (rs->PreSimulation && !rs->PreSimulation->IsClosed()) {
                lua_pushnumber(Lm, now);
                lua_pushnumber(Lm, dt);
                rs->PreSimulation->Fire(Lm, lua_gettop(Lm)-1, 2);
                lua_pop(Lm, 2);
            }

            PhysicsSimulation();

            if (rs->PostSimulation && !rs->PostSimulation->IsClosed()) {
                lua_pushnumber(Lm, dt);
                rs->PostSimulation->Fire(Lm, lua_gettop(Lm), 1);
                lua_pop(Lm, 1);
            }
            if (rs->Heartbeat && !rs->Heartbeat->IsClosed()) {
                lua_pushnumber(Lm, dt);
                rs->Heartbeat->Fire(Lm, lua_gettop(Lm), 1);
                lua_pop(Lm, 1);
            }
        }
        if (g_game && g_game->luaScheduler)
            g_game->luaScheduler->Step(GetTime(), dt);

        // Update UserInputService
        // IM ABOUT TO ROTTING AITHGSFODJgmarzsfoidlkzgj;,rsdfplgl;jars.kzf/dkgpksdzl LET ME fUCKING SLEEEP ALREADY
        // WHY I HAVE TO STAY HERE,  ICOMING HERE AND I CHECK THE SIGNAL I CHECK SCHEDULAR I WANT TO FUCKING KILL MYSELF BROOOOOOOOOOOOOOOOOOOOOOOO LET ME GOO
        auto uis = std::dynamic_pointer_cast<UserInputService>(Service::Get("UserInputService"));
        if (uis) {
            uis->Update();
        }

        // Update TweenService
        auto tweenService = std::dynamic_pointer_cast<TweenService>(Service::Get("TweenService"));
        if (tweenService) {
            tweenService->Update(dt);
        }

        // Update GUI Manager
        if (g_guiManager) {
            g_guiManager->Update();
        }

        // Update camera from workspace CurrentCamera
        // put me out of the misery keep messing with this many times now
        if (g_game && g_game->workspace && g_game->workspace->camera) {
            auto currentCamera = g_game->workspace->camera;
            CFrame renderCFrame = currentCamera->GetRenderCFrame();
            
            // Convert CFrame to raylib Camera3D
            g_camera.position = renderCFrame.p.toRay();
            g_camera.target = Vector3Add(g_camera.position, renderCFrame.lookVector().toRay());
            g_camera.up = renderCFrame.upVector().toRay();
            g_camera.fovy = currentCamera->FieldOfView;
        }

        RenderFrame(g_camera);
        
        // Render GUI on top of everything
        if (g_guiManager) {
            g_guiManager->Render();
            
            // Show instruction when GUI is not visible
            if (!g_guiManager->IsGuiVisible()) {
                int screenHeight = GetScreenHeight();
                if (GuiManager::IsCustomFontLoaded()) {
                    DrawTextEx(GuiManager::GetCustomFont(), "Press '5' five times to open editor", {10, (float)(screenHeight - 30)}, 16, 1, LIGHTGRAY);
                } else {
                    DrawText("Press '5' five times to open editor", 10, screenHeight - 30, 16, LIGHTGRAY);
                }
            }
        }
        
        EndDrawing();
    }

    LOGI("Stage: Run loop end");
}

static void Cleanup() {
    LOGI("Cleanup begin");
    if (g_guiManager) {
        g_guiManager->Shutdown();
        g_guiManager.reset();
    }
    if (g_game) {
        g_game->Shutdown();
        g_game.reset();
    }
    CloseWindow();
    LOGI("Cleanup end");
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--target-fps") == 0 && i + 1 < argc) {
            gTargetFPS = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--path") == 0 && i + 1 < argc) {
            gPaths.push_back(argv[++i]);
            args = true;
        } else if (std::strcmp(argv[i], "--no-place") == 0) {
            gNoPlace = true;
        } else if (i == 1) {
            // first non-flag argument
            std::string arg = argv[i];
            if (arg.find(' ') == std::string::npos &&
                arg.size() > 4 &&
                arg.rfind(".lua") == arg.size() - 4) {
                gPaths.push_back(arg);
                args = true;
            }
        }
    }

    Stage_ConfigInitialization();

    if (!Preflight_ValidateResources()) {
        return EXIT_FAILURE; // exit if resources are missing
    }

    if (!Preflight_ValidatePaths()) {
        return EXIT_FAILURE; // print error then exit before window/renderer
    }

    LOGI("-----------------------------");
    LOGI("ECLIPSERA ENGINE (Client)");
    LOGI("License: MIT license");
    LOGI("Author: LunarEngine Developers, NickiBreeki (modification)");
    LOGI("-----------------------------");

    Stage_Initialization();

    if (gTargetFPS > 0) {
        SetTargetFPS(gTargetFPS);
        LOGI("Target FPS set to %d", gTargetFPS);
    }

    Stage_Run();
    return 0;
}
