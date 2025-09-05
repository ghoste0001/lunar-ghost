#include "bootstrap/instances/MeshPart.h"
#include "core/logging/Logging.h"
#include <cstring>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <raylib.h>
#include <raymath.h>

MeshPart::MeshPart(std::string name)
    : BasePart(std::move(name), InstanceClass::MeshPart) {

    // so uh roblox meshes behavior is something like, you inserted the mesh
    // and the blank mesh size is the same as part size and then
    // when the mesh loads it getting the actual mesh size applied to it

    Size = {2.0f, 2.0f, 2.0f};
    MeshSize = {1.0f, 1.0f, 1.0f};
    HasLoadedMesh = false;
    LoadedModel = {0};
    LOGI("MeshPart created '%s'", Name.c_str());
}

MeshPart::~MeshPart() {
    if (HasLoadedMesh && LoadedModel.meshCount > 0) {
        UnloadModel(LoadedModel);
        LoadedModel = {0};
        HasLoadedMesh = false;
    }

    if (LoadedTexture.id > 0) {
        UnloadTexture(LoadedTexture);
        LoadedTexture = {0};
    }
}

bool MeshPart::LuaGet(lua_State* L, const char* key) const {
    // Handle MeshPart-specific properties first
    if (std::strcmp(key, "DoubleSided") == 0) {
        lua_pushboolean(L, DoubleSided);
        return true;
    }
    if (std::strcmp(key, "MeshId") == 0) {
        lua_pushstring(L, MeshId.c_str());
        return true;
    }
    if (std::strcmp(key, "TextureID") == 0) {
        lua_pushstring(L, TextureID.c_str()); // i hate working on this
        return true;
    }
    if (std::strcmp(key, "CollisionFidelity") == 0) {
        lua_pushinteger(L, CollisionFidelity);
        return true;
    }
    if (std::strcmp(key, "RenderFidelity") == 0) {
        lua_pushinteger(L, RenderFidelity);
        return true;
    }
    if (std::strcmp(key, "FluidFidelity") == 0) {
        lua_pushinteger(L, FluidFidelity);
        return true;
    }
    if (std::strcmp(key, "MeshSize") == 0) {
        lb::push(L, Vector3Game::fromRay(MeshSize));
        return true;
    }
    if (std::strcmp(key, "HasJointOffset") == 0) {
        lua_pushboolean(L, HasJointOffset);
        return true;
    }
    if (std::strcmp(key, "HasSkinnedMesh") == 0) {
        lua_pushboolean(L, HasSkinnedMesh);
        return true;
    }
    if (std::strcmp(key, "JointOffset") == 0) {
        lb::push(L, Vector3Game::fromRay(JointOffset));
        return true;
    }
    
    // Fall back to BasePart properties
    return BasePart::LuaGet(L, key);
}

bool MeshPart::LuaSet(lua_State* L, const char* key, int valueIndex) {
    // Handle MeshPart-specific properties first
    if (std::strcmp(key, "DoubleSided") == 0) {
        DoubleSided = lua_toboolean(L, valueIndex);
        return true;
    }
    if (std::strcmp(key, "MeshId") == 0) {
        const char* meshId = luaL_checkstring(L, valueIndex);
        MeshId = meshId ? meshId : "";
        LOGI("MeshPart '%s': Setting MeshId to '%s'", Name.c_str(), MeshId.c_str());
        loadMesh();
        return true;
    }
    if (std::strcmp(key, "TextureID") == 0) {
        const char* textureId = luaL_checkstring(L, valueIndex);
        TextureID = textureId ? textureId : "";
        LOGI("MeshPart '%s': Setting TextureID to '%s'", Name.c_str(), TextureID.c_str());
        loadTexture();
        return true;
    }
    if (std::strcmp(key, "CollisionFidelity") == 0) {
        CollisionFidelity = (int)luaL_checkinteger(L, valueIndex);
        if (CollisionFidelity < 0) CollisionFidelity = 0;
        if (CollisionFidelity > 2) CollisionFidelity = 2;
        return true;
    }
    if (std::strcmp(key, "RenderFidelity") == 0) {
        RenderFidelity = (int)luaL_checkinteger(L, valueIndex);
        if (RenderFidelity < 0) RenderFidelity = 0;
        if (RenderFidelity > 2) RenderFidelity = 2;
        return true;
    }
    if (std::strcmp(key, "FluidFidelity") == 0) {
        FluidFidelity = (int)luaL_checkinteger(L, valueIndex);
        if (FluidFidelity < 0) FluidFidelity = 0;
        if (FluidFidelity > 2) FluidFidelity = 2;
        return true;
    }
    
    // Read-only properties - ignore attempts to set them
    if (std::strcmp(key, "MeshSize") == 0 ||
        std::strcmp(key, "HasJointOffset") == 0 ||
        std::strcmp(key, "HasSkinnedMesh") == 0 ||
        std::strcmp(key, "JointOffset") == 0) {
        return true;
    }
    
    return BasePart::LuaSet(L, key, valueIndex);
}

void MeshPart::ApplyMesh(const MeshPart* sourceMeshPart) {
    if (!sourceMeshPart) {
        LOGW("ApplyMesh called with null source MeshPart");
        return;
    }
    
    MeshId = sourceMeshPart->MeshId;
    TextureID = sourceMeshPart->TextureID;
    RenderFidelity = sourceMeshPart->RenderFidelity;
    CollisionFidelity = sourceMeshPart->CollisionFidelity;
    FluidFidelity = sourceMeshPart->FluidFidelity;
    MeshSize = sourceMeshPart->MeshSize;
    
    loadMesh();
    
    LOGI("Applied mesh from '%s' to '%s'", sourceMeshPart->Name.c_str(), Name.c_str());
}

bool MeshPart::loadMesh() {
    if (HasLoadedMesh && LoadedModel.meshCount > 0) {
        UnloadModel(LoadedModel);
        LoadedModel = {0};
        HasLoadedMesh = false;
    }
    
    if (MeshId.empty()) {
        LOGI("MeshPart '%s': No MeshId specified", Name.c_str());
        MeshSize = {1.0f, 1.0f, 1.0f};
        return false;
    }
    
    LOGI("MeshPart '%s': Loading mesh '%s'", Name.c_str(), MeshId.c_str());
    
    // Allowed OBJ for now, later add FBX, GLTF, etc. im lazy yknow..
    if (MeshId.find(".obj") != std::string::npos) {
        return loadOBJFile();
    }
    
    LOGW("MeshPart '%s': Unsupported mesh format for '%s'", Name.c_str(), MeshId.c_str());
    return false;
}

bool MeshPart::loadOBJFile() {
    // Try multiple possible paths
    // This is a bit hacky but works for now
    // In future, consider a more robust resource management system
    // that can handle URLs, asset libraries, etc.
    std::vector<std::string> pathsToTry = {
        MeshId,                                    // Direct path
        "resources/" + MeshId,                     // In exe resources folder
        "../resources/" + MeshId,                  // Up one level then resources
        "../../eclipsera-engine/build/resources/" + MeshId,  // From examples to build/resources
        "../eclipsera-engine/build/resources/" + MeshId,     // From root to build/resources
        "eclipsera-engine/build/resources/" + MeshId,        // From root to build/resources
        "build/resources/" + MeshId,               // From eclipsera-engine to build/resources
        "examples/" + MeshId,                      // In examples folder
        "../examples/" + MeshId,                   // Up one level then examples
        "../../examples/" + MeshId,                // Up two levels then examples
        "./" + MeshId,                            // Current directory
        "../" + MeshId                            // Up one level
    };
    
    for (const std::string& path : pathsToTry) {
        LOGI("MeshPart '%s': Trying to load OBJ from '%s'", Name.c_str(), path.c_str());
        
        if (loadOBJFromPath(path)) {
            LOGI("MeshPart '%s': Successfully loaded OBJ from '%s'", Name.c_str(), path.c_str());
            return true;
        }
    }
    
    LOGW("MeshPart '%s': Failed to load OBJ file '%s' from any path", Name.c_str(), MeshId.c_str());
    return false;
}

bool MeshPart::loadOBJFromPath(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    // "SIMPLE" OBJ parser - collect vertices and faces
    // do you mean S1MPLE the one csgo pro player?

    // Who the fuck is S1MPLE???

    // just-fucking-google.it
    std::vector<Vector3> vertices;
    std::vector<Vector3> normals;
    std::vector<Vector2> texCoords;
    std::vector<int> vertexIndices;
    std::vector<int> normalIndices;
    std::vector<int> texIndices;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            // Vertex position
            Vector3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(vertex);
        }
        else if (prefix == "vn") {
            // Vertex normal
            Vector3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (prefix == "vt") {
            // Texture coordinate
            Vector2 texCoord;
            iss >> texCoord.x >> texCoord.y;
            texCoords.push_back(texCoord);
        }
        else if (prefix == "f") {
            // Face - parse up to 4 vertices (triangulate quads)
            std::string vertexData;
            std::vector<std::string> faceVertices;
            
            while (iss >> vertexData) {
                faceVertices.push_back(vertexData);
            }
            
            // Triangulate if needed (convert quads to triangles)
            if (faceVertices.size() >= 3) {
                // First triangle
                for (int i = 0; i < 3; i++) {
                    parseVertexData(faceVertices[i], vertexIndices, texIndices, normalIndices);
                }
                
                // If quad, create second triangle
                if (faceVertices.size() == 4) {
                    parseVertexData(faceVertices[0], vertexIndices, texIndices, normalIndices);
                    parseVertexData(faceVertices[2], vertexIndices, texIndices, normalIndices);
                    parseVertexData(faceVertices[3], vertexIndices, texIndices, normalIndices);
                }
            }
        }
    }
    
    file.close();
    
    if (vertices.empty() || vertexIndices.empty()) {
        LOGW("MeshPart '%s': No valid geometry found in OBJ file", Name.c_str());
        return false;
    }
    
    // Create raylib mesh
    return createRaylibMesh(vertices, normals, texCoords, vertexIndices, normalIndices, texIndices);
}

void MeshPart::parseVertexData(const std::string& vertexData, 
                              std::vector<int>& vertexIndices,
                              std::vector<int>& texIndices, 
                              std::vector<int>& normalIndices) {
    std::istringstream iss(vertexData);
    std::string indexStr;
    
    // Parse vertex index
    if (std::getline(iss, indexStr, '/')) {
        int vIndex = std::stoi(indexStr) - 1; // OBJ is 1-based
        vertexIndices.push_back(vIndex);
    }
    
    // Parse texture index
    if (std::getline(iss, indexStr, '/')) {
        if (!indexStr.empty()) {
            int tIndex = std::stoi(indexStr) - 1;
            texIndices.push_back(tIndex);
        } else {
            texIndices.push_back(-1);
        }
    } else {
        texIndices.push_back(-1);
    }
    
    // Parse normal index
    if (std::getline(iss, indexStr)) {
        if (!indexStr.empty()) {
            int nIndex = std::stoi(indexStr) - 1;
            normalIndices.push_back(nIndex);
        } else {
            normalIndices.push_back(-1);
        }
    } else {
        normalIndices.push_back(-1);
    }
}

bool MeshPart::createRaylibMesh(const std::vector<Vector3>& vertices,
                               const std::vector<Vector3>& normals,
                               const std::vector<Vector2>& texCoords,
                               const std::vector<int>& vertexIndices,
                               const std::vector<int>& normalIndices,
                               const std::vector<int>& texIndices) {
    
    int triangleCount = vertexIndices.size() / 3;
    int vertexCount = vertexIndices.size();
    
    if (triangleCount == 0) {
        LOGW("MeshPart '%s': No triangles to create", Name.c_str());
        return false;
    }
    
    // Create raylib Mesh
    Mesh mesh = {0};
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = triangleCount;
    
    // Allocate arrays
    mesh.vertices = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
    mesh.normals = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(vertexCount * 2 * sizeof(float));
    
    // Calculate bounding box
    Vector3 minBounds = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 maxBounds = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    
    // Fill vertex data
    for (int i = 0; i < vertexCount; i++) {
        int vIndex = vertexIndices[i];
        int nIndex = (i < normalIndices.size()) ? normalIndices[i] : -1;
        int tIndex = (i < texIndices.size()) ? texIndices[i] : -1;
        
        // Vertex position
        if (vIndex >= 0 && vIndex < vertices.size()) {
            Vector3 v = vertices[vIndex];
            mesh.vertices[i * 3 + 0] = v.x;
            mesh.vertices[i * 3 + 1] = v.y;
            mesh.vertices[i * 3 + 2] = v.z;
            
            // Update bounding box
            if (v.x < minBounds.x) minBounds.x = v.x;
            if (v.y < minBounds.y) minBounds.y = v.y;
            if (v.z < minBounds.z) minBounds.z = v.z;
            if (v.x > maxBounds.x) maxBounds.x = v.x;
            if (v.y > maxBounds.y) maxBounds.y = v.y;
            if (v.z > maxBounds.z) maxBounds.z = v.z;
        } else {
            mesh.vertices[i * 3 + 0] = 0.0f;
            mesh.vertices[i * 3 + 1] = 0.0f;
            mesh.vertices[i * 3 + 2] = 0.0f;
        }
        
        // Vertex normal
        if (nIndex >= 0 && nIndex < normals.size()) {
            Vector3 n = normals[nIndex];
            mesh.normals[i * 3 + 0] = n.x;
            mesh.normals[i * 3 + 1] = n.y;
            mesh.normals[i * 3 + 2] = n.z;
        } else {
            // Generate normal from triangle if not provided
            if (i % 3 == 0 && i + 2 < vertexCount) {
                Vector3 v0 = {mesh.vertices[i * 3], mesh.vertices[i * 3 + 1], mesh.vertices[i * 3 + 2]};
                Vector3 v1 = {mesh.vertices[(i + 1) * 3], mesh.vertices[(i + 1) * 3 + 1], mesh.vertices[(i + 1) * 3 + 2]};
                Vector3 v2 = {mesh.vertices[(i + 2) * 3], mesh.vertices[(i + 2) * 3 + 1], mesh.vertices[(i + 2) * 3 + 2]};
                
                Vector3 edge1 = Vector3Subtract(v1, v0);
                Vector3 edge2 = Vector3Subtract(v2, v0);
                Vector3 normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));
                
                // Apply to all 3 vertices of the triangle
                for (int j = 0; j < 3; j++) {
                    mesh.normals[(i + j) * 3 + 0] = normal.x;
                    mesh.normals[(i + j) * 3 + 1] = normal.y;
                    mesh.normals[(i + j) * 3 + 2] = normal.z;
                }
            }
        }
        
        // Texture coordinates (flip V axis for OBJ -> OpenGL)
        if (tIndex >= 0 && tIndex < texCoords.size()) {
            Vector2 t = texCoords[tIndex];
            mesh.texcoords[i * 2 + 0] = t.x;
            mesh.texcoords[i * 2 + 1] = 1.0f - t.y; // flip vertically
        } else {
            // Fallback: assign a simple tiling UV pattern
            mesh.texcoords[i * 2 + 0] = (float)(i % 2);
            mesh.texcoords[i * 2 + 1] = (float)((i / 2) % 2);
        }
    }
    
    // Update MeshSize based on bounding box
    MeshSize.x = maxBounds.x - minBounds.x;
    MeshSize.y = maxBounds.y - minBounds.y;
    MeshSize.z = maxBounds.z - minBounds.z;
    
    // Ensure minimum size not minimum wage
    // minimum wage???
    if (MeshSize.x < 0.1f) MeshSize.x = 0.1f;
    if (MeshSize.y < 0.1f) MeshSize.y = 0.1f;
    if (MeshSize.z < 0.1f) MeshSize.z = 0.1f;
    
    // Upload mesh to GPU
    GenMeshTangents(&mesh);
    UploadMesh(&mesh, false);
    
    // Create Model from Mesh
    LoadedModel = LoadModelFromMesh(mesh);
    
    // Ensure the model has the correct mesh count and pointer
    if (LoadedModel.meshCount == 0) {
        LoadedModel.meshCount = 1;
    }
    if (LoadedModel.meshes == nullptr) {
        LoadedModel.meshes = &mesh;
    }
    
    // Set up material
    if (LoadedModel.materialCount > 0) {
        LoadedModel.materials[0] = LoadMaterialDefault();
    }
    
    HasLoadedMesh = true;
    
    LOGI("MeshPart '%s': Created mesh with %d vertices, %d triangles", 
         Name.c_str(), vertexCount, triangleCount);
    LOGI("MeshPart '%s': MeshSize = (%.2f, %.2f, %.2f)", 
         Name.c_str(), MeshSize.x, MeshSize.y, MeshSize.z);
    LOGI("MeshPart '%s': LoadedModel.meshCount = %d, meshes = %p, HasLoadedMesh = %s", 
         Name.c_str(), LoadedModel.meshCount, LoadedModel.meshes, HasLoadedMesh ? "true" : "false");
    
    // If we have a TextureID..
    // try to load and apply the texture now that the mesh is loaded
    // idk how to handling this properly. i think this is fine ig? if it not load just load the file again :troll_face:
    if (!TextureID.empty()) {
        LOGI("MeshPart '%s': Mesh loaded, now applying texture '%s'", Name.c_str(), TextureID.c_str());
        loadTexture();
    }
    
    return true;
}

bool MeshPart::loadTexture() {
    if (TextureID.empty()) {
        LOGI("MeshPart '%s': No TextureID specified", Name.c_str());
        return false;
    }

    LOGI("MeshPart '%s': Loading texture '%s'", Name.c_str(), TextureID.c_str());

    std::vector<std::string> pathsToTry = {
        TextureID,
        "resources/" + TextureID,
        "../resources/" + TextureID,
        "../../eclipsera-engine/build/resources/" + TextureID,
        "../eclipsera-engine/build/resources/" + TextureID,
        "eclipsera-engine/build/resources/" + TextureID,
        "build/resources/" + TextureID,
        "examples/" + TextureID,
        "../examples/" + TextureID,
        "../../examples/" + TextureID,
        "./" + TextureID,
        "../" + TextureID
    };

    for (const std::string& path : pathsToTry) {
        LOGI("MeshPart '%s': Trying to load texture from '%s'", Name.c_str(), path.c_str());

        if (FileExists(path.c_str())) {
            // Load texture into class member to keep it alive
            LoadedTexture = LoadTexture(path.c_str());

            if (LoadedTexture.id > 0) {
                LOGI("MeshPart '%s': Successfully loaded texture from '%s' (ID: %d, %dx%d)", 
                    Name.c_str(), path.c_str(), LoadedTexture.id, LoadedTexture.width, LoadedTexture.height);

                // APPLY THE TEXTURE TO THE MESH
                if (HasLoadedMesh) {
                    if (LoadedModel.materialCount == 0) {
                        LoadedModel.materialCount = 1;
                        LoadedModel.materials = (Material*)MemAlloc(sizeof(Material));
                        LoadedModel.materials[0] = LoadMaterialDefault();
                    }

                    LoadedModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = LoadedTexture;
                    LoadedModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {255,255,255,255};

                    LOGI("MeshPart '%s': Applied texture to material", Name.c_str());
                }

                return true;
            } else {
                LOGW("MeshPart '%s': Failed to load texture from '%s' (invalid texture ID)", Name.c_str(), path.c_str());
            }
        }
    }

    LOGW("MeshPart '%s': Failed to load texture '%s' from any path", Name.c_str(), TextureID.c_str());
    return false;
}

// Register MeshPart with the Instance system
static Instance::Registrar _reg_meshpart("MeshPart", [] {
    return std::make_shared<MeshPart>("MeshPart");
});
