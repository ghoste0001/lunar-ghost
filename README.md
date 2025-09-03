<img src="/repo/IconNLabelCompressed.svg" width="512"/>

# ECLIPSERA ENGINE (DEPRECATED)
Eclipsera Engine is a modified version of [LunarEngine](https://github.com/lunarengine/lunar-test), designed to include features that the original project currently lacks. This engine is built using ROBLOX's Luau and aims to support the full range of Roblox APIs for game development. and Freedom of Game Development

## **PRONOUNCIATION**
E-CLIP-SERA EN-GINE, E-CRIP-SERA EN-JIN, E-KRIP-SERRAH EN-JEAN, Yī-kè-lǐ-pǔ-sī-lā Ān-jīn, 伊克利普斯拉 安金

> [!NOTE]
> The project status is now changed from **work in progress** to **deprecated**
> > **Here's note:**
hi there, i hope you wouldn't be sad to see this, but i really had to do it, due to my cpp limitation has come to the end, i may not able to continue working on this because i had a hard time understand stuff, and something i unable to do, it very difficult to me, and i trying to pushed it to the true limit and i think i am at the true limit now, The file right here that you'll get is a unfinish code, if you want to continue after me, i do appreciated for your contribution, loved you all ♥️

# ❓ Got Question?
If you got any questions, Feel free to leave it in Discussions! or find your question in this section!
- https://github.com/n1ckibreeki/Eclipsera-Engine/wiki/#❓-FAQ---Frequently-Asked-Questions

## How can i reported bug? Or give feedback?
- You can report any suspected bugs in the Issues section.
  - https://github.com/n1ckibreeki/Eclipsera-Engine/issues
- For general feedback or suggestions, it is best to leave them in the Discussions tab.
  - https://github.com/n1ckibreeki/Eclipsera-Engine/discussions

---
## SERVICE AVAILABILITY
This section provides an overview of which services are currently available in the engine and which ones are still under development or planned for the future. It is intended to help developers understand the current state of the engine's features and make informed decisions when building projects. For a detailed, up-to-date status of each service, please refer to the full list here
- https://github.com/n1ckibreeki/Eclipsera-Engine/wiki/⚙-Services-Status

## Compatibility
Eclipsera Engine is currently in its demo phase as LunarEngine, implementing a limited but functional subset of its intended API. While it's not feature-complete, it already supports a wide range of basic engine functionality enough to begin experimenting with rendering, scripting, and basic scene construction.

<video src="/repo/Parts.mp4" width="500px"></video>

### Scene Rendering
- Basic scene rendering is functional, enabling real-time visualization of 3D environments.
- Includes lighting system support, featuring:
  - `[Ambient lighting]:` Simulates overall light presence in a scene.
  - `[Shadows]:` Dynamic or static, depending on lighting settings.
  - `[Skybox rendering]:` Background visuals to create atmospheric depth.
- Supports rendering of objects parented under `game.Workspace`, including `BasePart` instances.
- Basic camera control is implemented, allowing for simple navigation or player view movement.
Currently still uses the LunarEngine `"Libre-1"` rendering pipeline
### Instance & Object System
- Supports nearly the full Instance API, modeled after Roblox’s architecture.
- Exceptions: Currently missing :WaitForChild() support.
- Key instance methods and properties available:
  - `.Parent` (for parenting to hierarchies)
  - `:Destroy()`, `:Clone()` (object lifecycle control)
### Part Implementation
- Fully supports BasePart creation via
```lua
Instance.new("Part")
```
- Property support includes:
  - `[Visuals]:` `BasePart.Color`, `BasePart.Transparency`
  - `[Size and positioning]:` `BasePart.Size`, `BasePart.Position`, `BasePart.CFrame`
> Future updates will add support for other instance types like `Model`, `MeshPart`, `Decal`, etc.
### Core Data Types & Globals
- Implements support for key standard data types: `CFrame`, `Vector3`, `Color3`, Random`
- Core global objects and services: `game`, `script`, `workspace` are usable
- `game:GetService()` for service retrieval is functional
### Lighting System
The Lighting service is partially implemented, supporting real-time ambient and environmental adjustments:
  - `Lighting.Ambient` - sets global light tint
  - `ighting.ShadowSoftness` - adjusts the contrast and blur of shadows
  - `Lighting.ClockTime` - simulates in-game time of day
  - `Lighting.Brightness` - controls scene brightness
These can be dynamically changed at runtime and affect all visible objects.
### RunService & Render Pipeline
Full support for the five standard RunService stages: `RenderStepped`, `Heartbeat`, `Stepped`, etc.
Allows connections and waits:
```lua
- RunService.RenderStepped:Connect(function...)
- RunService.RenderStepped:Wait()
```
- Scripts can be synchronized with these stages for frame-accurate updates.
### Program Execution Engine (Luau Support)
Eclipse Engine includes strong support for Luau scripting, with several powerful features:
- Full support for:
  - `Scripts`, `LocalScripts`, `Coroutines`, `task.spawn`, `task.wait`, `task.delay`
- Custom task scheduler named Hyperball manages script execution efficiently
  - Prioritizes stable, non-blocking task management
  - Built-in support for signals and event-based programming
- Event handling via:
  - `RTScriptSignal`, `:Connect()`, `:Disconnect()` style listeners
- Optimized Luau runtime enabled by default for improved performance

---

## Current Features
- Separated shader files: `shaders/shaders.h` and `shaders/sky.h` have been split from `Renderer.cpp` for better organization.
- Basic support for the Roblox API, including `UserInputService`, `TweenService`
> (that only service that LunarEngine doesn't have it rn, be patience, i will add more but might take a time, i'm no expert at cpp language lol)
- Early-stage support for GLSL shaders.
- Performance improvements, Fixed a crash issue in the Lua scheduler that occurred when loading large LuaU files.
> (Note: creating over 100,000 instances at once still causes crashes. this is currently a work in progress.)
- Luau Based Camera Handler
- Luau Script Type Inside of Lua `--$serverside`, `--$clientside`, `--$modular (wait i think i deprecate this already had no plan continue working on this, so no appearance..)`

## Roadmap 🏳
- Add a new instance/class called `Skybox`.
- Provide full GLSL shader support, along with legacy/basic post-processing options that can be used in the `LightingService` without requiring prior knowledge of GLSL.
- Implement an ImGUI-based debugging tool to support development. This will include access to an Explorer panel, Properties, memory usage stats, and hardware information.
- Rewrite LunarEngine’s ambient occlusion system using a modern technique, with customizable settings available through the `LightingService`.
  
# License©
- Lua, licensed under the MIT License. | Copyright (c) 1998 Lua.org.
  - https://www.lua.org/, https://github.com/lua/lua
- Luau, licensed under the MIT License. | Copyright (c) 2025 Roblox Corporation.
  - https://luau.org/ , https://github.com/luau-lang/luau
- Raylib, licensed under the zlib/libpng License. | Copyright (c) 2013-2025 Ramon Santamaria and Contributors.
  - https://www.raylib.com/ , https://github.com/raysan5/raylib
- LunarEngine, licensed under the MIT License. | Copyright (c) 2025  LunarEngine Development.
  - https://github.com/lunarengine/lunar-test/
