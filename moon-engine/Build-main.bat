@echo off

setlocal EnableExtensions
setlocal EnableDelayedExpansion

rem #############################################################################
rem # MoonEngine Build Script
rem #
rem # Usage:
rem #   build.bat           - (Fastest) Builds only what has changed.
rem #   build.bat -rebuild  - Cleans everything and builds from scratch.
rem #   build.bat -debug    - Creates a debug build.
rem #
rem #############################################################################

rem --- Set up the Visual Studio Developer Environment ---
if not defined VSCMD_ARG_TGT_ARCH (
  call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
  if %errorlevel% neq 0 (
    echo [ERROR] Could not initialize the Visual Studio developer environment.
    exit /b 1
  )
)
echo ==========================================================
echo.
echo   MMM   MMM OOOOOOOO OOOOOOOO NNN   NN
echo   MMMM MMMM OO    OO OO    OO NNNN  NN
echo   MM MMM MM OO    OO OO    OO NN NN NN
echo   MM  M  MM OO    OO OO    OO NN  NNNN
echo   MM     MM OOOOOOOO OOOOOOOO NN  NNNN
echo.
echo   MODIFICATION OF LUNAR-ENGINE.
echo   BY GHOSTE.
echo.
echo ==========================================================

rem --- Configuration ---
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%"
set "BUILD_DIR=%PROJECT_ROOT%build"
set "DIST_DIR=%PROJECT_ROOT%dist"
set "CMAKE_PREFIX_PATH=%PROJECT_ROOT%third_party\vendor"

rem --- Default Settings ---
set "DO_REBUILD=0"
set "CMAKE_BUILD_TYPE=Release"

rem --- Parse Command-Line Arguments ---
for %%a in (%*) do (
    if /i "%%a"=="-rebuild" set "DO_REBUILD=1"
    if /i "%%a"=="-debug" set "CMAKE_BUILD_TYPE=Debug"
)

rem --- Dynamically set parallel build level to the number of CPU cores ---
set "PARALLEL_LEVEL=%NUMBER_OF_PROCESSORS%"
echo.
echo Build Type: %CMAKE_BUILD_TYPE%
echo Parallel Jobs: %PARALLEL_LEVEL%
echo.

rem --- 1. Clean (only if -rebuild is specified) ---
echo [1/4] Cleaning...
if "%DO_REBUILD%"=="1" (
  echo      -rebuild flag detected. Deleting build and dist directories.
  if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"
  if exist "%DIST_DIR%" rd /s /q "%DIST_DIR%"
) else (
  echo      Skipping clean for a faster incremental build. Use -rebuild for a full clean.
)

rem --- 2. Configure (CMake) ---
echo.
echo [2/4] Configuring Moon Engine...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
pushd "%BUILD_DIR%"

rem Only run the full CMake configuration if necessary
if "%DO_REBUILD%"=="1" or not exist "CMakeCache.txt" (
    echo      Running CMake configuration...
    cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%"
    if %errorlevel% neq 0 ( echo.[ERROR] CMake configuration failed. & popd & exit /b 1 )
) else (
    echo      CMake cache found. Skipping configuration step.
)

rem --- 3. Build (Ninja) ---
echo.
echo [3/4] Building MoonEngine (%CMAKE_BUILD_TYPE%)...

cmake --build . --config %CMAKE_BUILD_TYPE% --parallel %PARALLEL_LEVEL%

if %errorlevel% neq 0 ( echo.[ERROR] MoonEngine build failed. & popd & exit /b 1 )

rem --- 4. Install ---
echo.
echo [4/4] Installing to 'dist'...
cmake --install . --prefix "%DIST_DIR%"
if %errorlevel% neq 0 ( echo.[ERROR] Installation failed. & popd & exit /b 1 )

popd
echo.
echo ==========================================================
echo               BUILD SUCCESSFUL
echo ==========================================================