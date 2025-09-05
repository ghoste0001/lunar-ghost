# CMakeLists.txt (Project Root)
cmake_minimum_required(VERSION 3.20)
project(moon-engine LANGUAGES C CXX)

# MSVC runtime (/MD)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# Paths
set(PROJ_ROOT "${CMAKE_SOURCE_DIR}")
set(VENDOR_DIR "${PROJ_ROOT}/third_party/vendor")

# ---------- Find Pre-built Luau lib ----------
set(LUAU_INSTALL_DIR "${VENDOR_DIR}/luau")
find_library(LUAU_LIB
  NAMES Luau
  PATHS "${LUAU_INSTALL_DIR}/lib"
  NO_DEFAULT_PATH
)
if(NOT LUAU_LIB)
  message(FATAL_ERROR "Luau library not found. Did you run build_dependencies.bat? Searched in: ${LUAU_INSTALL_DIR}/lib")
endif()
message(STATUS "Found Luau library: ${LUAU_LIB}")


# ---------- Find Pre-built Raylib lib ----------
set(RAYLIB_INSTALL_DIR "${VENDOR_DIR}/raylib")
find_library(RAYLIB_LIB
  NAMES raylib raylib_static
  PATHS "${RAYLIB_INSTALL_DIR}/lib"
  NO_DEFAULT_PATH
)
if(NOT RAYLIB_LIB)
  message(FATAL_ERROR "raylib(.lib) not found in ${RAYLIB_INSTALL_DIR}/lib")
endif()
message(STATUS "Found Raylib library: ${RAYLIB_LIB}")




# ---------- Engine exe ----------
file(GLOB_RECURSE ENGINE_SOURCES
  "${PROJ_ROOT}/bootstrap/*.cpp"
  "${PROJ_ROOT}/core/*.cpp"
  "${PROJ_ROOT}/services/*.cpp"
  "${PROJ_ROOT}/subsystems/*.cpp"
)
add_executable(moon-engine ${ENGINE_SOURCES})

# ---vvv--- THE FIX IS HERE ---vvv---
# We now point to the specific subdirectories where the headers were installed,
# exactly like your original build script did.
target_include_directories(moon-engine PRIVATE
  "${PROJ_ROOT}"
  "${LUAU_INSTALL_DIR}/include/luau/Common/include"
  "${LUAU_INSTALL_DIR}/include/luau/Ast/include"
  "${LUAU_INSTALL_DIR}/include/luau/Compiler/include"
  "${LUAU_INSTALL_DIR}/include/luau/Config/include"
  "${LUAU_INSTALL_DIR}/include/luau/VM/include"
  "${RAYLIB_INSTALL_DIR}/include"
)
# ---^^^--- THE FIX IS HERE ---^^^---

target_compile_features(moon-engine PRIVATE cxx_std_20)

if(MSVC)
  target_compile_options(moon-engine PRIVATE /EHsc /O2 /DNOMINMAX)
endif()


# ---------- Linking ----------
target_link_libraries(moon-engine PRIVATE ${LUAU_LIB} ${RAYLIB_LIB})
if(WIN32)
  target_link_libraries(moon-engine PRIVATE opengl32 gdi32 winmm user32 shell32)
endif()


# ---------- Installation (The new "Dist" step) ----------
install(TARGETS moon-engine
  RUNTIME DESTINATION .
)
set(CONTENT_SRC "${PROJ_ROOT}/content/dist")
if(EXISTS "${CONTENT_SRC}")
  install(DIRECTORY "${CONTENT_SRC}/"
    DESTINATION .
  )
endif()

# Install resources folder
set(RESOURCES_SRC "${PROJ_ROOT}/bootstrap/resources")
if(EXISTS "${RESOURCES_SRC}")
  install(DIRECTORY "${RESOURCES_SRC}/"
    DESTINATION resources
  )
endif()

# Copy resources folder to build directory during build
add_custom_command(TARGET moon-engine POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${RESOURCES_SRC}"
    "$<TARGET_FILE_DIR:moon-engine>/resources"
  COMMENT "Copying resources folder to build directory"
)

# Add version resource on Windows
if(WIN32)
  target_sources(moon-engine PRIVATE "${PROJ_ROOT}/moon-engine.rc")
endif()

# Set final output name
set_target_properties(moon-engine PROPERTIES OUTPUT_NAME "MoonEngine")
