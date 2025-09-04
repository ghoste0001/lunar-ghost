#pragma once
#include "bootstrap/instances/BasePart.h"
#include <string>
#include <vector>

// Forward declare Lua and raylib types
struct lua_State;
struct Model; // Forward declare raylib Model
struct Vector3;
struct Vector2;

struct MeshPart : BasePart {
    // MeshPart-specific properties
    bool DoubleSided{false};
    std::string MeshId{""};
    std::string TextureID{""};
    
    // Collision and rendering fidelity enums (simplified as ints for now)
    int CollisionFidelity{2}; // 0=Box, 1=Hull, 2=Default
    int RenderFidelity{0};    // 0=Automatic, 1=Precise, 2=Performance
    int FluidFidelity{2};     // 0=Box, 1=Hull, 2=Default
    // idk how it works but let's just do something like this fine ig?? idk my idea idk how roblox handle those
    
    // Read-only mesh size (updated when mesh is loaded)
    ::Vector3 MeshSize{1.0f, 1.0f, 1.0f};
    
    // Internal properties (hidden/not accessible in normal usage)
    bool HasJointOffset{false};
    bool HasSkinnedMesh{false};
    
    ::Vector3 JointOffset{0.0f, 0.0f, 0.0f};
    
    // Renderer compatibility properties
    bool HasLoadedMesh{false};
    Model LoadedModel{}; // raylib Model for rendering
    Texture2D LoadedTexture = {0};
    ::Vector3 Offset{0.0f, 0.0f, 0.0f};

    MeshPart(std::string name = "MeshPart");
    ~MeshPart() override;

    // Override Lua interface to handle MeshPart-specific properties
    bool LuaGet(lua_State* L, const char* key) const override;
    bool LuaSet(lua_State* L, const char* key, int valueIndex) override;
    
    // MeshPart-specific methods
    void ApplyMesh(const MeshPart* sourceMeshPart);
    
private:
    // Helper methods
    bool loadMesh();
    bool loadTexture();
    bool loadOBJFile();
    bool loadOBJFromPath(const std::string& filepath);
    void parseVertexData(const std::string& vertexData, 
                        std::vector<int>& vertexIndices,
                        std::vector<int>& texIndices, 
                        std::vector<int>& normalIndices);
    bool createRaylibMesh(const std::vector<Vector3>& vertices,
                         const std::vector<Vector3>& normals,
                         const std::vector<Vector2>& texCoords,
                         const std::vector<int>& vertexIndices,
                         const std::vector<int>& normalIndices,
                         const std::vector<int>& texIndices);
};
