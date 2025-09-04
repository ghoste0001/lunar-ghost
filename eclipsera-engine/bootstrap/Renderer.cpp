// Renderer.cpp
#include "shaders/shaders.h"
#include "shaders/sky.h"

#include "Renderer.h"
#include "Renderer.h"
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <cfloat>
#include <unordered_map>
#include <cstdint>
#include "bootstrap/Game.h"
#include "bootstrap/instances/InstanceTypes.h"
#include "bootstrap/instances/BasePart.h"      // for CF
#include "bootstrap/instances/MeshPart.h"      // for MeshPart
#include "core/datatypes/CFrame.h"             // for CF
#include "core/logging/Logging.h"              // for LOGI
#include "bootstrap/ShapeMeshes.h"             // for shape vertex data
#include "bootstrap/services/Lighting.h"      // for Lighting service
#include "bootstrap/services/Service.h"       // for Service::Get
extern std::shared_ptr<Game> g_game;


// i'll tell you a truth while working on shadering, i fucking losing it, so i put whole code to the ai to read it and tell me
// how to do a shader work and shit tons of work so i fucking mess this shit up and the light is break into pieces
// i wished you could enjoy reading all the mess that i did here, have a good one!

// ---------------- Tunables ----------------
static float kMaxDrawDistance = 10000.0f;
static float kFovPaddingDeg   = 20.0f;

// shadow parameter definitions
static float kShadowMaxDistance = 300.0f;  // increased shadow distance
static int   kShadowRes         = 2048;    // higher resolution shadows
static float kPCFStep           = 0.8f;    // smoother PCF
static bool  kCullBackFace      = true;
static bool  kStabilizeShadow   = true;    // snap to texel grid to prevent swimming

// CSM controls
static int   kNumCascades       = 3;       // use all 3 cascades
static float kCameraNear        = 0.1f;    // closer near plane

// Enhanced lighting controls
static bool     kUseClockTime = true;
static float    kClockTime    = 14.0f;     // afternoon lighting
static float    kBrightness   = 4.0f;      // brighter sun for better shadow contrast
static float    kExposure     = 1.0f;      // higher exposure for brighter scene
static ::Color3 kAmbient      = {0.5f, 0.5f, 0.6f}; // brighter ambient for better visibility

// Shadow debugging and runtime controls (inspired by Raylib example)
static bool     kShowShadowDebug = false;  // show shadow cascade splits
static bool     kEnableShadows   = true;   // toggle shadows on/off
static float    kShadowBiasMultiplier = 1.0f; // adjust shadow bias dynamically

// New hybrid rendering features
static bool     kEnableSSAO    = true;     // Screen-space ambient occlusion
static bool     kEnableBloom   = true;     // HDR bloom effect
static bool     kEnableFog     = true;     // Atmospheric fog
static float    kFogDensity    = 0.02f;    // Fog density
static ::Color3 kFogColor      = {0.7f, 0.8f, 0.9f}; // Light blue fog
static bool     kEnableVolumetricLighting = true;    // God rays
static float    kVolumetricStrength = 0.3f;          // Volumetric light strength

static inline float LenSq(Vector3 v){ return v.x*v.x + v.y*v.y + v.z*v.z; }
struct SavedWin { int x,y,w,h; bool valid=false; } g_saved;

inline ::Color ToRaylibColor(const ::Color3& c, float alpha = 1.0f) {
    return {
        (unsigned char)std::lroundf(std::clamp(c.r, 0.0f, 1.0f) * 255.0f),
        (unsigned char)std::lroundf(std::clamp(c.g, 0.0f, 1.0f) * 255.0f),
        (unsigned char)std::lroundf(std::clamp(c.b, 0.0f, 1.0f) * 255.0f),
        (unsigned char)std::lroundf(std::clamp(alpha, 0.0f, 1.0f) * 255.0f)
    };
}

static void EnterBorderlessFullscreen(int monitor = -1) {
    if (!g_saved.valid) {
        Vector2 p = GetWindowPosition();
        g_saved = { (int)p.x, (int)p.y, GetScreenWidth(), GetScreenHeight(), true };
    }
    if (monitor < 0) monitor = GetCurrentMonitor();
    Vector2 mpos = GetMonitorPosition(monitor);
    int mw = GetMonitorWidth(monitor), mh = GetMonitorHeight(monitor);
    SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);
    SetWindowSize(mw, mh);
    SetWindowPosition((int)mpos.x, (int)mpos.y);
}
static void ExitBorderlessFullscreen() {
    ClearWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);
    if (g_saved.valid) { SetWindowSize(g_saved.w, g_saved.w); SetWindowPosition(g_saved.x, g_saved.y); }
    else RestoreWindow();
}

// ---- Helpers ----
// Exact parity at ClockTime = 12.0  ->  normalize(1, -1, 1)
static Vector3 SunDirFromClock(float clockHours){
    float t = fmodf(fmaxf(clockHours, 0.0f), 24.0f);
    const float phi = PI*0.25f; // fixed azimuth 45°
    const float alphaNoon = asinf(1.0f / sqrtf(3.0f)); // ≈35.264°
    float w = (2.0f*PI/24.0f) * (t - 6.0f);            // 6->sunrise, 12->noon, 18->sunset
    float alpha = alphaNoon * sinf(w);                 // elevation
    float ca = cosf(alpha), sa = sinf(alpha);
    float cphi = cosf(phi),  sphi = sinf(phi);
    return { ca*cphi, -sa, ca*sphi };                  // from sun to scene
}

// ---------------- Shader objects / models / shadow RT ----------------
static Shader gLitShader   = {0};
static Shader gLitShaderInst = {0};
static Shader gSkyShader   = {0};
static Shader gPostProcessShader = {0};  // Post-processing shader
static Shader gBloomShader = {0};        // Bloom effect shader
static Model  gPartModel   = {0}; // cube model used for parts
static Model  gSkyModel    = {0};
static Material gPartMatInst = {0};
static RenderTexture2D gShadowMapCSM[3] = { {0},{0},{0} };

// Cache for shape-specific models
static std::unordered_map<int, Model> gShapeModels;

// Post-processing render targets
static RenderTexture2D gMainRT = {0};     // Main scene render target
static RenderTexture2D gBloomRT = {0};    // Bloom render target
static RenderTexture2D gTempRT = {0};     // Temporary render target for effects

// Post-processing uniforms
static int u_fogDensity = -1, u_fogColor = -1;
static int u_volumetricStrength = -1;
static int u_bloomThreshold = -1, u_bloomIntensity = -1;

// Sky uniforms
static int u_inner=-1, u_outer=-1, u_transition=-1;

// Lit shader uniform locations
static int u_sunDir=-1, u_sky=-1, u_ground=-1, u_hemi=-1, u_sun=-1;
static int u_spec=-1, u_shiny=-1, u_fresnel=-1, u_viewPos=-1;
static int u_aoStrength=-1, u_groundY=-1, u_ambient=-1;

// CSM uniforms
static int u_lightVP0=-1, u_lightVP1=-1, u_lightVP2=-1;
static int u_shadowMap0=-1, u_shadowMap1=-1, u_shadowMap2=-1;
static int u_shadowRes=-1, u_pcfStep=-1, u_biasMin=-1, u_biasMax=-1;
static int u_splits=-1;
static int u_exposure = -1;
static int u_normalBias0=-1, u_normalBias1=-1, u_normalBias2=-1;

// Instanced shader uniform locations (mirror)
static int ui_sunDir=-1, ui_sky=-1, ui_ground=-1, ui_hemi=-1, ui_sun=-1;
static int ui_spec=-1, ui_shiny=-1, ui_fresnel=-1, ui_viewPos=-1;
static int ui_aoStrength=-1, ui_groundY=-1, ui_ambient=-1;
static int ui_lightVP0=-1, ui_lightVP1=-1, ui_lightVP2=-1;
static int ui_shadowMap0=-1, ui_shadowMap1=-1, ui_shadowMap2=-1;
static int ui_shadowRes=-1, ui_pcfStep=-1, ui_biasMin=-1, ui_biasMax=-1;
static int ui_splits=-1;
static int ui_exposure=-1;
static int ui_normalBias0=-1, ui_normalBias1=-1, ui_normalBias2=-1;
static int ui_transition = -1;

// ---------------- Shadowmap helpers ----------------
static RenderTexture2D LoadShadowmapRenderTexture(int width, int height){
    RenderTexture2D target = {0};
    target.id = rlLoadFramebuffer(); // empty FBO
    target.texture.width = width;
    target.texture.height = height;

    if (target.id > 0){
        rlEnableFramebuffer(target.id);

        // Depth texture only
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19; // DEPTH24
        target.depth.mipmaps = 1;

        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferComplete(target.id);
        rlDisableFramebuffer();
    }
    return target;
}

static void UnloadShadowmapRenderTexture(RenderTexture2D target){
    if (target.id > 0) rlUnloadFramebuffer(target.id);
}

// --------- CFrame -> axis-angle (for DrawModelEx) ----------
static inline float clampf(float x, float a, float b){ return x < a ? a : (x > b ? b : x); }

static void CFrameToAxisAngle(const CFrame& cf, ::Vector3& axisOut, float& angleDegOut){
    // Row-major 3x3
    const float m00 = cf.R[0], m01 = cf.R[1], m02 = cf.R[2];
    const float m10 = cf.R[3], m11 = cf.R[4], m12 = cf.R[5];
    const float m20 = cf.R[6], m21 = cf.R[7], m22 = cf.R[8];

    const float trace = m00 + m11 + m22;
    float cosA = (trace - 1.0f)*0.5f;
    cosA = clampf(cosA, -1.0f, 1.0f);
    float angle = acosf(cosA);

    if (angle < 1e-6f){
        axisOut = {0.0f, 1.0f, 0.0f};
        angleDegOut = 0.0f;
        return;
    }

    if (fabsf(PI - angle) < 1e-4f){
        // Near 180°, robust axis extraction
        float xx = sqrtf(fmaxf(0.0f, (m00 + 1.0f)*0.5f));
        float yy = sqrtf(fmaxf(0.0f, (m11 + 1.0f)*0.5f));
        float zz = sqrtf(fmaxf(0.0f, (m22 + 1.0f)*0.5f));
        // pick signs from off-diagonals
        xx = copysignf(xx, m21 - m12);
        yy = copysignf(yy, m02 - m20);
        zz = copysignf(zz, m10 - m01);
        float len = sqrtf(xx*xx + yy*yy + zz*zz);
        if (len < 1e-6f) { axisOut = {0.0f,1.0f,0.0f}; angleDegOut = 180.0f; return; }
        axisOut = { xx/len, yy/len, zz/len };
        angleDegOut = 180.0f;
        return;
    }

    float s = 2.0f*sinf(angle);
    ::Vector3 axis = {
        (m21 - m12) / s,
        (m02 - m20) / s,
        (m10 - m01) / s
    };
    float len = sqrtf(axis.x*axis.x + axis.y*axis.y + axis.z*axis.z);
    if (len < 1e-6f) { axisOut = {0.0f,1.0f,0.0f}; angleDegOut = 0.0f; return; }
    axisOut = { axis.x/len, axis.y/len, axis.z/len };
    angleDegOut = angle * (180.0f/PI);
}

// ---------------- Dynamic shadow helpers ----------------

// Return a safe 'up' vector for the given direction
static inline Vector3 SafeUpForDir(Vector3 d){
    Vector3 up = {0,1,0};
    if (fabsf(Vector3DotProduct(up, d)) > 0.99f) up = {0,0,1};
    return up;
}

// Compute world-space frustum corners for camera slice [nearD, farD]
static void GetFrustumCornersWS(const Camera3D& cam, float nearD, float farD, Vector3 outCorners[8]){
    const float vFov = cam.fovy * (PI/180.0f);
    const float aspect = (float)GetScreenWidth()/(float)GetScreenHeight();

    Vector3 F = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 R = Vector3Normalize(Vector3CrossProduct(F, cam.up));
    Vector3 U = Vector3CrossProduct(R, F);

    float hN = tanf(vFov*0.5f)*nearD, wN = hN*aspect;
    float hF = tanf(vFov*0.5f)*farD,  wF = hF*aspect;

    Vector3 cN = Vector3Add(cam.position, Vector3Scale(F, nearD));
    Vector3 cF = Vector3Add(cam.position, Vector3Scale(F, farD));

    // near
    outCorners[0] = Vector3Add(Vector3Add(cN, Vector3Scale(U,  hN)), Vector3Scale(R,  wN)); // ntr
    outCorners[1] = Vector3Add(Vector3Add(cN, Vector3Scale(U,  hN)), Vector3Scale(R, -wN)); // ntl
    outCorners[2] = Vector3Add(Vector3Add(cN, Vector3Scale(U, -hN)), Vector3Scale(R,  wN)); // nbr
    outCorners[3] = Vector3Add(Vector3Add(cN, Vector3Scale(U, -hN)), Vector3Scale(R, -wN)); // nbl
    // far
    outCorners[4] = Vector3Add(Vector3Add(cF, Vector3Scale(U,  hF)), Vector3Scale(R,  wF)); // ftr
    outCorners[5] = Vector3Add(Vector3Add(cF, Vector3Scale(U,  hF)), Vector3Scale(R, -wF)); // ftl
    outCorners[6] = Vector3Add(Vector3Add(cF, Vector3Scale(U, -hF)), Vector3Scale(R,  wF)); // fbr
    outCorners[7] = Vector3Add(Vector3Add(cF, Vector3Scale(U, -hF)), Vector3Scale(R, -wF)); // fbl
}

// Transform point by matrix (4x4 * vec4(p,1))
static inline Vector3 XformPoint(const Matrix& m, Vector3 p){
    float x = m.m0*p.x + m.m4*p.y + m.m8*p.z + m.m12;
    float y = m.m1*p.x + m.m5*p.y + m.m9*p.z + m.m13;
    float z = m.m2*p.x + m.m6*p.y + m.m10*p.z + m.m14;
    return { x, y, z };
}

// Build a directional light camera tightly fitting the camera frustum slice
// NOTE: outTexelWS returns the world-space size of one shadow map texel for this cascade.
static void BuildLightCameraForSlice(const Camera3D& cam, Vector3 sunDir,
                                     float sliceNear, float sliceFar,
                                     int shadowRes,
                                     int rtW, int rtH,
                                     Camera3D& outCam, Matrix& outLightVP,
                                     float& outTexelWS)
{
    Vector3 cornersWS[8];
    GetFrustumCornersWS(cam, sliceNear, sliceFar, cornersWS);

    // compute centroid
    Vector3 center = {0,0,0};
    for (int i=0;i<8;i++) center = Vector3Add(center, cornersWS[i]);
    center = Vector3Scale(center, 1.0f/8.0f);

    Vector3 upL = SafeUpForDir(Vector3Scale(sunDir, -1.0f));
    // initial view from sun direction
    Matrix lightView = MatrixLookAt(Vector3Add(center, Vector3Scale(Vector3Scale(sunDir, -1.0f), 200.0f)), center, upL);

    // compute bounds in light space
    Vector3 mn = { FLT_MAX, FLT_MAX, FLT_MAX };
    Vector3 mx = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    for (int i=0;i<8;i++){
        Vector3 p = XformPoint(lightView, cornersWS[i]);
        mn.x = fminf(mn.x, p.x); mn.y = fminf(mn.y, p.y); mn.z = fminf(mn.z, p.z);
        mx.x = fmaxf(mx.x, p.x); mx.y = fmaxf(mx.y, p.y); mx.z = fmaxf(mx.z, p.z);
    }

    // extents and aspect
    float halfW = 0.5f*(mx.x - mn.x);
    float halfH = 0.5f*(mx.y - mn.y);
    const float aspectRT = (float)rtW / (float)rtH;
    float orthoHalfY = fmaxf(halfH, halfW / aspectRT);
    float orthoHalfX = orthoHalfY * aspectRT;

    // center in light space
    Vector3 centerLS = { (mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f, (mn.z + mx.z) * 0.5f };

    // texel snap
    float texelW = (2.0f * orthoHalfX) / (float)shadowRes;
    float texelH = (2.0f * orthoHalfY) / (float)shadowRes;
    float texelWS = fmaxf(texelW, texelH);

    if (kStabilizeShadow && shadowRes > 0) {
        centerLS.x = floorf(centerLS.x / texelW) * texelW;
        centerLS.y = floorf(centerLS.y / texelH) * texelH;
    }

    // reproject snapped center to world
    Matrix invView = MatrixInvert(lightView);
    Vector3 snappedCenterWS = XformPoint(invView, centerLS);

    // final view
    lightView = MatrixLookAt(Vector3Add(snappedCenterWS, Vector3Scale(Vector3Scale(sunDir, -1.0f), 200.0f)), snappedCenterWS, upL);

    // depth range padded - tightened from 50 to 10
    const float zNear = -mx.z - 10.0f;
    const float zFar  = -mn.z + 10.0f;

    Matrix lightProj = MatrixOrtho(-orthoHalfX, +orthoHalfX, -orthoHalfY, +orthoHalfY, zNear, zFar);

    outLightVP = MatrixMultiply(lightView, lightProj);

    // fill Camera3D for BeginMode3D
    outCam = {0};
    outCam.projection = CAMERA_ORTHOGRAPHIC;
    outCam.up = upL;
    outCam.target = snappedCenterWS;
    outCam.position = Vector3Add(snappedCenterWS, Vector3Scale(Vector3Scale(sunDir, -1.0f), 200.0f));
    // raylib's ortho via fovy behaves differently: set fovy to full half-height to match our ortho extents
    outCam.fovy = orthoHalfY * 2.0f;

    outTexelWS = texelWS;
}

// ---------------- Init ----------------
static void EnsureShaders() {
    if (!gLitShader.id) {
        gLitShader = LoadShaderFromMemory(LIT_VS, LIT_FS);

        // lighting
        u_aoStrength = GetShaderLocation(gLitShader, "aoStrength");
        ui_aoStrength = GetShaderLocation(gLitShaderInst, "aoStrength");

        u_viewPos   = GetShaderLocation(gLitShader, "viewPos");
        u_sunDir    = GetShaderLocation(gLitShader, "sunDir");
        u_sky       = GetShaderLocation(gLitShader, "skyColor");
        u_ground    = GetShaderLocation(gLitShader, "groundColor");
        u_hemi      = GetShaderLocation(gLitShader, "hemiStrength");
        u_sun       = GetShaderLocation(gLitShader, "sunStrength");
        u_ambient   = GetShaderLocation(gLitShader, "ambientColor");
        u_spec      = GetShaderLocation(gLitShader, "specStrength");
        u_shiny     = GetShaderLocation(gLitShader, "shininess");
        u_fresnel   = GetShaderLocation(gLitShader, "fresnelStrength");
        u_aoStrength= GetShaderLocation(gLitShader, "aoStrength");
        u_groundY   = GetShaderLocation(gLitShader, "groundY");

        // cascades
        u_lightVP0  = GetShaderLocation(gLitShader, "lightVP0");
        u_lightVP1  = GetShaderLocation(gLitShader, "lightVP1");
        u_lightVP2  = GetShaderLocation(gLitShader, "lightVP2");
        u_shadowMap0= GetShaderLocation(gLitShader, "shadowMap0");
        u_shadowMap1= GetShaderLocation(gLitShader, "shadowMap1");
        u_shadowMap2= GetShaderLocation(gLitShader, "shadowMap2");
        u_shadowRes = GetShaderLocation(gLitShader, "shadowMapResolution");
        u_pcfStep   = GetShaderLocation(gLitShader, "pcfStep");
        u_biasMin   = GetShaderLocation(gLitShader, "biasMin");
        u_biasMax   = GetShaderLocation(gLitShader, "biasMax");
        u_splits    = GetShaderLocation(gLitShader, "cascadeSplits");
        u_transition = GetShaderLocation(gLitShader, "transitionFrac");

        // normal bias
        u_normalBias0 = GetShaderLocation(gLitShader, "normalBiasWS0");
        u_normalBias1 = GetShaderLocation(gLitShader, "normalBiasWS1");
        u_normalBias2 = GetShaderLocation(gLitShader, "normalBiasWS2");

        // exposure
        u_exposure  = GetShaderLocation(gLitShader, "exposure");

        // constants
        SetShaderValue(gLitShader, u_shadowRes, &kShadowRes, SHADER_UNIFORM_INT);
        float pcfStep = kPCFStep;
        SetShaderValue(gLitShader, u_pcfStep, &pcfStep, SHADER_UNIFORM_FLOAT);
        
        // Dynamic shadow bias (inspired by Raylib example)
        float biasMin = 6e-5f * kShadowBiasMultiplier;
        float biasMax = 8e-4f * kShadowBiasMultiplier;
        SetShaderValue(gLitShader, u_biasMin, &biasMin, SHADER_UNIFORM_FLOAT);
        SetShaderValue(gLitShader, u_biasMax, &biasMax, SHADER_UNIFORM_FLOAT);

        // set a default exposure in case it's not updated per-frame
        SetShaderValue(gLitShader, u_exposure, &kExposure, SHADER_UNIFORM_FLOAT);

        float defaultTransition = 0.15f; // 15% of split distance -> smooth blend
        SetShaderValue(gLitShader, u_transition, &defaultTransition, SHADER_UNIFORM_FLOAT);
    }

    if (!gLitShaderInst.id) {
        gLitShaderInst = LoadShaderFromMemory(LIT_VS_INST, LIT_FS);

        // in order for DrawMeshInstanced to pick up instanceTransform as the model matrix attribute,
        // assign its attribute location to SHADER_LOC_MATRIX_MODEL
        gLitShaderInst.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(gLitShaderInst, "instanceTransform");

        // lighting (instanced)
        ui_viewPos   = GetShaderLocation(gLitShaderInst, "viewPos");
        ui_sunDir    = GetShaderLocation(gLitShaderInst, "sunDir");
        ui_sky       = GetShaderLocation(gLitShaderInst, "skyColor");
        ui_ground    = GetShaderLocation(gLitShaderInst, "groundColor");
        ui_hemi      = GetShaderLocation(gLitShaderInst, "hemiStrength");
        ui_sun       = GetShaderLocation(gLitShaderInst, "sunStrength");
        ui_ambient   = GetShaderLocation(gLitShaderInst, "ambientColor");
        ui_spec      = GetShaderLocation(gLitShaderInst, "specStrength");
        ui_shiny     = GetShaderLocation(gLitShaderInst, "shininess");
        ui_fresnel   = GetShaderLocation(gLitShaderInst, "fresnelStrength");
        ui_aoStrength= GetShaderLocation(gLitShaderInst, "aoStrength");
        ui_groundY   = GetShaderLocation(gLitShaderInst, "groundY");

        // cascades (instanced)
        ui_lightVP0  = GetShaderLocation(gLitShaderInst, "lightVP0");
        ui_lightVP1  = GetShaderLocation(gLitShaderInst, "lightVP1");
        ui_lightVP2  = GetShaderLocation(gLitShaderInst, "lightVP2");
        ui_shadowMap0= GetShaderLocation(gLitShaderInst, "shadowMap0");
        ui_shadowMap1= GetShaderLocation(gLitShaderInst, "shadowMap1");
        ui_shadowMap2= GetShaderLocation(gLitShaderInst, "shadowMap2");
        ui_shadowRes = GetShaderLocation(gLitShaderInst, "shadowMapResolution");
        ui_pcfStep   = GetShaderLocation(gLitShaderInst, "pcfStep");
        ui_biasMin   = GetShaderLocation(gLitShaderInst, "biasMin");
        ui_biasMax   = GetShaderLocation(gLitShaderInst, "biasMax");
        ui_splits    = GetShaderLocation(gLitShaderInst, "cascadeSplits");
        ui_transition= GetShaderLocation(gLitShaderInst, "transitionFrac");

        // normal bias (instanced)
        ui_normalBias0 = GetShaderLocation(gLitShaderInst, "normalBiasWS0");
        ui_normalBias1 = GetShaderLocation(gLitShaderInst, "normalBiasWS1");
        ui_normalBias2 = GetShaderLocation(gLitShaderInst, "normalBiasWS2");

        // exposure
        ui_exposure  = GetShaderLocation(gLitShaderInst, "exposure");

        // constants
        SetShaderValue(gLitShaderInst, ui_shadowRes, &kShadowRes, SHADER_UNIFORM_INT);
        float pcfStep2 = kPCFStep;
        SetShaderValue(gLitShaderInst, ui_pcfStep, &pcfStep2, SHADER_UNIFORM_FLOAT);
        
        // Dynamic shadow bias (inspired by Raylib example)
        float biasMin2 = 6e-5f * kShadowBiasMultiplier;
        float biasMax2 = 8e-4f * kShadowBiasMultiplier;
        SetShaderValue(gLitShaderInst, ui_biasMin, &biasMin2, SHADER_UNIFORM_FLOAT);
        SetShaderValue(gLitShaderInst, ui_biasMax, &biasMax2, SHADER_UNIFORM_FLOAT);
        float defaultTransition2 = 0.15f;
        SetShaderValue(gLitShaderInst, ui_transition, &defaultTransition2, SHADER_UNIFORM_FLOAT);
        SetShaderValue(gLitShaderInst, ui_exposure, &kExposure, SHADER_UNIFORM_FLOAT);
    }

    if (!gSkyShader.id) {
        gSkyShader = LoadShaderFromMemory(SKY_VS, SKY_FS);
        u_inner = GetShaderLocation(gSkyShader, "innerColor");
        u_outer = GetShaderLocation(gSkyShader, "outerColor");
    }

// Shared cube model for all parts (default Block shape)
if (gPartModel.meshCount == 0) {
    Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    gPartModel = LoadModelFromMesh(cubeMesh);
    gPartModel.materials[0].shader = gLitShader; // bind lit+shadow shader for non-instanced path
}

    // Sky cube
    if (gSkyModel.meshCount == 0) {
        Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
        gSkyModel = LoadModelFromMesh(cubeMesh);
        gSkyModel.materials[0].shader = gSkyShader;
    }

    // instanced material
    if (!gPartMatInst.shader.id) {
        gPartMatInst = LoadMaterialDefault();
        gPartMatInst.shader = gLitShaderInst;
    }

    for (int i=0;i<3;i++){
        if (!gShadowMapCSM[i].id) {
            gShadowMapCSM[i] = LoadShadowmapRenderTexture(kShadowRes, kShadowRes);
        }
    }
}

// ---------------- Utility: compute cascade splits ----------------
static inline void ComputeCascadeSplits(float n, float f, float lambda, float outSplit[2]){
    const int C = 3;
    for (int i=1;i<=C-1;i++){
        float si = (float)i / (float)C;
        float logD = n * powf(f/n, si);
        float linD = n + (f - n) * si;
        float d = lambda * logD + (1.0f - lambda) * linD;
        outSplit[i-1] = d;
    }
}

// ---------------- Helper: Generate mesh based on shape ----------------
static Mesh GenerateMeshForShape(int shape) {
    switch (shape) {
        case 0: // Ball
            return GenMeshSphere(0.5f, 16, 16);
        case 1: // Block (default cube)
            return GenMeshCube(1.0f, 1.0f, 1.0f);
        case 2: // Cylinder
            {
                Mesh cylinder = GenMeshCylinder(0.5f, 1.0f, 16);
                
                // Center the cylinder properly - Raylib's GenMeshCylinder creates a cylinder
                // that extends from 0 to height, but we want it centered around origin? yeah right
                if (cylinder.vertices) {
                    for (int i = 0; i < cylinder.vertexCount; i++) {
                        // Shift Y coordinates to center the cylinder around origin
                        cylinder.vertices[i * 3 + 1] -= 0.5f; // Move down by half height
                    }
                    
                    // Update the mesh on GPU
                    UpdateMeshBuffer(cylinder, 0, cylinder.vertices, cylinder.vertexCount * 3 * sizeof(float), 0);
                }
                
                return cylinder;
            }
        case 3: // Wedge - create a ramp-like wedge mesh using header data, i'm not enjoy working on this
            {
                Mesh wedge = {0};
                wedge.vertexCount = ShapeMeshes::WEDGE_VERTEX_COUNT;
                wedge.triangleCount = ShapeMeshes::WEDGE_TRIANGLE_COUNT;
                
                // Allocate memory for vertices, normals, and texcoords
                wedge.vertices = (float*)MemAlloc(wedge.vertexCount * 3 * sizeof(float));
                wedge.normals = (float*)MemAlloc(wedge.vertexCount * 3 * sizeof(float));
                wedge.texcoords = (float*)MemAlloc(wedge.vertexCount * 2 * sizeof(float));
                
                // Copy vertices from header file
                for (int i = 0; i < wedge.vertexCount * 3; i++) {
                    wedge.vertices[i] = ShapeMeshes::WEDGE_VERTICES[i];
                }
                
                // Generate normals for each face
                for (int i = 0; i < wedge.vertexCount; i += 3) {
                    Vector3 v1 = {wedge.vertices[i*3], wedge.vertices[i*3+1], wedge.vertices[i*3+2]};
                    Vector3 v2 = {wedge.vertices[(i+1)*3], wedge.vertices[(i+1)*3+1], wedge.vertices[(i+1)*3+2]};
                    Vector3 v3 = {wedge.vertices[(i+2)*3], wedge.vertices[(i+2)*3+1], wedge.vertices[(i+2)*3+2]};
                    // DUDE STOP INVERTED :sob:
                    Vector3 edge1 = Vector3Subtract(v2, v1);
                    Vector3 edge2 = Vector3Subtract(v3, v1);
                    Vector3 normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));
                    
                    for (int j = 0; j < 3; j++) {
                        wedge.normals[(i+j)*3] = normal.x;
                        wedge.normals[(i+j)*3+1] = normal.y;
                        wedge.normals[(i+j)*3+2] = normal.z;
                    }
                }
                
                // Generate texture coordinates
                for (int i = 0; i < wedge.vertexCount; i++) {
                    wedge.texcoords[i*2] = (wedge.vertices[i*3] + 0.5f);
                    wedge.texcoords[i*2+1] = (wedge.vertices[i*3+2] + 0.5f);
                }
                
                // Upload mesh data to GPU
                UploadMesh(&wedge, false);
                return wedge;
            }
        case 4: // CornerWedge - create a corner wedge mesh using header data
            {
                Mesh cornerWedge = {0};
                cornerWedge.vertexCount = ShapeMeshes::CORNER_WEDGE_VERTEX_COUNT;
                cornerWedge.triangleCount = ShapeMeshes::CORNER_WEDGE_TRIANGLE_COUNT;
                
                cornerWedge.vertices = (float*)MemAlloc(cornerWedge.vertexCount * 3 * sizeof(float));
                cornerWedge.normals = (float*)MemAlloc(cornerWedge.vertexCount * 3 * sizeof(float));
                cornerWedge.texcoords = (float*)MemAlloc(cornerWedge.vertexCount * 2 * sizeof(float));
                
                // Copy vertices from header file
                for (int i = 0; i < cornerWedge.vertexCount * 3; i++) {
                    cornerWedge.vertices[i] = ShapeMeshes::CORNER_WEDGE_VERTICES[i];
                }
                
                // Generate normals
                for (int i = 0; i < cornerWedge.vertexCount; i += 3) {
                    Vector3 v1 = {cornerWedge.vertices[i*3], cornerWedge.vertices[i*3+1], cornerWedge.vertices[i*3+2]};
                    Vector3 v2 = {cornerWedge.vertices[(i+1)*3], cornerWedge.vertices[(i+1)*3+1], cornerWedge.vertices[(i+1)*3+2]};
                    Vector3 v3 = {cornerWedge.vertices[(i+2)*3], cornerWedge.vertices[(i+2)*3+1], cornerWedge.vertices[(i+2)*3+2]};
                    
                    Vector3 edge1 = Vector3Subtract(v2, v1);
                    Vector3 edge2 = Vector3Subtract(v3, v1);
                    Vector3 normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));
                    
                    for (int j = 0; j < 3; j++) {
                        cornerWedge.normals[(i+j)*3] = normal.x;
                        cornerWedge.normals[(i+j)*3+1] = normal.y;
                        cornerWedge.normals[(i+j)*3+2] = normal.z;
                    }
                }
                
                // Generate texture coordinates
                for (int i = 0; i < cornerWedge.vertexCount; i++) {
                    cornerWedge.texcoords[i*2] = (cornerWedge.vertices[i*3] + 0.5f);
                    cornerWedge.texcoords[i*2+1] = (cornerWedge.vertices[i*3+2] + 0.5f);
                }
                
                // idk how you guys handle this but i'm fucked up rn, my brain is dead ass shit

                UploadMesh(&cornerWedge, false);
                return cornerWedge;
            }
        default:
            return GenMeshCube(1.0f, 1.0f, 1.0f);
    }
}

// ---------------- Helper: Build instance matrix ----------------
static inline Matrix BuildInstanceMatrix(const CFrame& cf, ::Vector3 size){
    // CFrame::R is row-major 3x3 as used above
    const float m00 = cf.R[0], m01 = cf.R[1], m02 = cf.R[2];
    const float m10 = cf.R[3], m11 = cf.R[4], m12 = cf.R[5];
    const float m20 = cf.R[6], m21 = cf.R[7], m22 = cf.R[8];
    ::Vector3 t = cf.p.toRay();

    Matrix M = {0};
    // column 0 = Sx * R[:,0]
    M.m0 = m00*size.x; M.m1 = m10*size.x; M.m2 = m20*size.x; M.m3 = 0.0f;
    // column 1 = Sy * R[:,1]
    M.m4 = m01*size.y; M.m5 = m11*size.y; M.m6 = m21*size.y; M.m7 = 0.0f;
    // column 2 = Sz * R[:,2]
    M.m8 = m02*size.z; M.m9 = m12*size.z; M.m10= m22*size.z; M.m11= 0.0f;
    // column 3 = translation
    M.m12 = t.x; M.m13 = t.y; M.m14 = t.z; M.m15 = 1.0f;
    return M;
}

// ---------------- Main render ----------------
void RenderFrame(Camera3D& camera) {
    // Fullscreen toggle
    if (IsKeyPressed(KEY_F11)) {
        static bool borderless=false; borderless=!borderless;
        if (borderless) EnterBorderlessFullscreen(); else ExitBorderlessFullscreen();
    }
    
    // Shadow debugging controls (inspired by Raylib example)
    if (IsKeyPressed(KEY_F1)) kShowShadowDebug = !kShowShadowDebug;
    if (IsKeyPressed(KEY_F2)) kEnableShadows = !kEnableShadows;
    if (IsKeyDown(KEY_LEFT_BRACKET)) kShadowBiasMultiplier = fmaxf(0.1f, kShadowBiasMultiplier - 0.1f);
    if (IsKeyDown(KEY_RIGHT_BRACKET)) kShadowBiasMultiplier = fminf(5.0f, kShadowBiasMultiplier + 0.1f);

    EnsureShaders();

    // Camera + culling
    const Vector3 camPos = camera.position;
    Vector3 camDir = Vector3Normalize(Vector3Subtract(camera.target, camPos));
    const float maxDistSq = kMaxDrawDistance * kMaxDrawDistance;
    const float vFov = camera.fovy * (PI/180.0f);
    const float aspect = (float)GetScreenWidth()/(float)GetScreenHeight();
    const float hFov = 2.0f * atanf(tanf(vFov*0.5f)*aspect);
    const float halfCone = 0.5f * ((hFov>vFov)?hFov:vFov) + kFovPaddingDeg*(PI/180.0f);
    const float cosHalf2 = cosf(halfCone)*cosf(halfCone); // kept for reference

    // Get lighting values from service (with fallbacks to constants)
    auto lightingService = std::dynamic_pointer_cast<Lighting>(Service::Get("Lighting"));
    float clockTime = lightingService ? (float)lightingService->ClockTime : kClockTime;
    float brightness = lightingService ? (float)lightingService->Brightness : kBrightness;
    ::Color3 ambientColor = lightingService ? lightingService->Ambient : kAmbient;

    // Lighting params
    Vector3 sunDirV = kUseClockTime
        ? SunDirFromClock(clockTime)
        : Vector3Normalize(Vector3{1.0f, -1.0f, 1.0f}); // original parity
    float sunDir[3] = { sunDirV.x, sunDirV.y, sunDirV.z };
    float sky[3]    = { 0.60f, 0.70f, 0.90f };
    float ground[3] = { 0.18f, 0.16f, 0.14f };
    float hemiStr   = 0.7f;
    float sunStr    = (brightness / 2.0f) * 0.6f;            // use service brightness
    float ambient[3]= { ambientColor.r, ambientColor.g, ambientColor.b }; // use service ambient
    float specStr   = 1.30f;
    float shininess = 128.0f;
    float fresnel   = 0.1f;
    float aoStr     = 0.9f; // Increased AO strength for more visible effect
    float groundY   = 0.5f;

    // Gather parts
    auto ws = g_game ? g_game->workspace : nullptr;
    struct TItem { std::shared_ptr<BasePart> p; float dist2; float alpha; };
    std::vector<std::shared_ptr<BasePart>> opaques;
    std::vector<TItem> transparents;

    if (ws) {
        for (const auto& p : ws->parts) {
            if (!p || !p->Alive) continue;

            // CF position
            Vector3 pos = p->CF.p.toRay();
            Vector3 delta = Vector3Subtract(pos, camPos);
            float d2 = LenSq(delta);
            if (d2 > maxDistSq) continue;

            float dist = sqrtf(d2);
            float cosTheta = Vector3DotProduct(camDir, delta) / (dist > 0 ? dist : 1.0f);

            // size-based bounding sphere radius
            float radius = 0.5f * Vector3Length(p->Size);

            // allow hit if center is inside cone OR sphere overlaps cone boundary
            float angleLimit = cosf(halfCone);
            // if (cosTheta < angleLimit && dist * 0.5f > radius) continue;

            float t = Clamp(p->Transparency, 0.0f, 1.0f);
            float a = 1.0f - t;
            if (a <= 0.0f) continue;
            if (a >= 1.0f) opaques.push_back(p);
            else transparents.push_back({p, d2, a});
        }
    }

    // ---------------- Shadow pass (3 cascades) ----------------
    Camera3D lightCam[3] = {{0},{0},{0}};
    Matrix lightVP[3] = { MatrixIdentity(), MatrixIdentity(), MatrixIdentity() };
    float cascadeTexelWS[3] = {0.0f, 0.0f, 0.0f};

    // compute splits in view-space distance
    float splitRaw[2] = {0.0f, 0.0f};
    ComputeCascadeSplits(kCameraNear, kShadowMaxDistance, kCameraNear==0.0f?0.5f:kCameraNear, splitRaw); // fallback safe call (though original used kCameraNear as near)
    ComputeCascadeSplits(kCameraNear, kShadowMaxDistance, 0.6f, splitRaw);
    float splits[3] = { splitRaw[0], splitRaw[1], kShadowMaxDistance };

    // build and render each cascade
    float nearD = kCameraNear;
    for (int i=0;i<3;i++){
        float farD = splits[i];
        BuildLightCameraForSlice(
            camera, sunDirV,
            nearD, farD,
            kShadowRes,
            gShadowMapCSM[i].texture.width,
            gShadowMapCSM[i].texture.height,
            lightCam[i], lightVP[i],
            cascadeTexelWS[i]
        );
        nearD = farD;
    }

    // Build instance transforms for shadow casters (include opaques and transparents)
    // Improved culling inspired by Raylib example - only cast shadows from nearby objects
    std::vector<Matrix> shadowXforms;
    shadowXforms.reserve(opaques.size() + transparents.size());
    
    float shadowCullDistSq = kShadowMaxDistance * kShadowMaxDistance;
    for (auto& p : opaques) {
        Vector3 pos = p->CF.p.toRay();
        Vector3 delta = Vector3Subtract(pos, camPos);
        float d2 = LenSq(delta);
        if (d2 <= shadowCullDistSq) { // Only cast shadows from objects within shadow distance
            shadowXforms.push_back(BuildInstanceMatrix(p->CF, p->Size));
        }
    }
    for (auto& it : transparents) {
        Vector3 pos = it.p->CF.p.toRay();
        Vector3 delta = Vector3Subtract(pos, camPos);
        float d2 = LenSq(delta);
        if (d2 <= shadowCullDistSq) { // Only cast shadows from objects within shadow distance
            shadowXforms.push_back(BuildInstanceMatrix(it.p->CF, it.p->Size));
        }
    }

    // Only render shadows if enabled (inspired by Raylib example)
    if (kEnableShadows) {
        for (int i=0;i<3;i++){
            BeginTextureMode(gShadowMapCSM[i]);
                ClearBackground(WHITE); // depth cleared by BeginMode3D
                BeginMode3D(lightCam[i]);

                    // note: rlGetMatrixModelview/projection gives the matrices raylib used; update our lightVP
                    Matrix lightView = rlGetMatrixModelview();
                    Matrix lightProj = rlGetMatrixProjection();
                    lightVP[i] = MatrixMultiply(lightView, lightProj);

                    // disable backface culling for shadow pass to reduce acne
                    rlDisableBackfaceCulling();

                    // Cast shadows from both opaque and transparent geometry (as solid)
                    if (!shadowXforms.empty()) {
                        // color doesn't matter; depth-only framebuffer will use depth
                        // ensure instanced material shader is active for drawing instanced meshes
                        gPartMatInst.shader = gLitShaderInst;
                        gPartMatInst.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
                        DrawMeshInstanced(gPartModel.meshes[0], gPartMatInst, shadowXforms.data(), (int)shadowXforms.size());
                    }

                    // re-enable culling to previous state
                    if (kCullBackFace) rlEnableBackfaceCulling();

                EndMode3D();
            EndTextureMode();
        }
    }

    // after building shadow maps, set per-cascade normal-bias based on texel size
    // choose ~1.5 texels of world-space offset as default
    float nb0 = 1.5f * cascadeTexelWS[0];
    float nb1 = 1.5f * cascadeTexelWS[1];
    float nb2 = 1.5f * cascadeTexelWS[2];

    SetShaderValue(gLitShader, u_normalBias0, &nb0, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gLitShader, u_normalBias1, &nb1, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gLitShader, u_normalBias2, &nb2, SHADER_UNIFORM_FLOAT);

    SetShaderValue(gLitShaderInst, ui_normalBias0, &nb0, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gLitShaderInst, ui_normalBias1, &nb1, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gLitShaderInst, ui_normalBias2, &nb2, SHADER_UNIFORM_FLOAT);

    // ---------------- Main pass ----------------
    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(camera);

    // Enhanced Skybox with atmospheric effects
    rlDisableDepthTest();
    rlDisableDepthMask();
    bool prevCull = kCullBackFace;
    if (kCullBackFace) rlDisableBackfaceCulling();
    BeginShaderMode(gSkyShader);
        // Dynamic sky colors based on sun position
        float sunHeight = sunDirV.y;
        float sunsetFactor = Clamp(1.0f - fabsf(sunHeight), 0.0f, 1.0f);
        
        // Interpolate between day and sunset colors
        float inner[3] = {
            Lerp(0.25f, 0.8f, sunsetFactor),   // More orange during sunset
            Lerp(0.45f, 0.4f, sunsetFactor),   // Less green during sunset
            Lerp(0.65f, 0.2f, sunsetFactor)    // Less blue during sunset
        };
        float outer[3] = {
            Lerp(0.05f, 0.3f, sunsetFactor),   // Darker orange at zenith
            Lerp(0.25f, 0.15f, sunsetFactor),  // Less green at zenith
            Lerp(0.55f, 0.1f, sunsetFactor)    // Much less blue at zenith
        };
        
        SetShaderValue(gSkyShader, u_inner, inner, SHADER_UNIFORM_VEC3);
        SetShaderValue(gSkyShader, u_outer, outer, SHADER_UNIFORM_VEC3);
        DrawModel(gSkyModel, {0,0,0}, 100.0f, WHITE);
    EndShaderMode();
    if (kCullBackFace && prevCull) rlEnableBackfaceCulling();
    rlEnableDepthMask();
    rlEnableDepthTest();

    if (kCullBackFace) rlEnableBackfaceCulling();

    // bind three depth textures to slots 10..12
    int slot0 = 10, slot1 = 11, slot2 = 12;
    rlActiveTextureSlot(slot0); rlEnableTexture(gShadowMapCSM[0].depth.id);
    rlActiveTextureSlot(slot1); rlEnableTexture(gShadowMapCSM[1].depth.id);
    rlActiveTextureSlot(slot2); rlEnableTexture(gShadowMapCSM[2].depth.id);
    
    // Tell shaders which texture slots to use for shadow maps (CRITICAL FIX!)
    SetShaderValue(gLitShader, u_shadowMap0, &slot0, SHADER_UNIFORM_INT);
    SetShaderValue(gLitShader, u_shadowMap1, &slot1, SHADER_UNIFORM_INT);
    SetShaderValue(gLitShader, u_shadowMap2, &slot2, SHADER_UNIFORM_INT);
    SetShaderValue(gLitShaderInst, ui_shadowMap0, &slot0, SHADER_UNIFORM_INT);
    SetShaderValue(gLitShaderInst, ui_shadowMap1, &slot1, SHADER_UNIFORM_INT);
    SetShaderValue(gLitShaderInst, ui_shadowMap2, &slot2, SHADER_UNIFORM_INT);

    // Prepare per-frame uniform values
    float viewPosArr[3] = { camera.position.x, camera.position.y, camera.position.z };
    float splitVec[3] = { splits[0], splits[1], splits[2] };
    float transitionFrac = 0.15f; // 0.05..0.25 typical; lower = tighter band
    float exposure = kExposure;

    // Helper to set per-frame uniforms for a shader (handles instanced vs non-instanced)
    auto SetPerFrame = [&](const Shader& sh, bool inst){
        // choose location set
        int loc_viewPos    = inst ? ui_viewPos    : u_viewPos;
        int loc_sunDir     = inst ? ui_sunDir     : u_sunDir;
        int loc_sky        = inst ? ui_sky        : u_sky;
        int loc_ground     = inst ? ui_ground     : u_ground;
        int loc_hemi       = inst ? ui_hemi       : u_hemi;
        int loc_sun        = inst ? ui_sun        : u_sun;
        int loc_ambient    = inst ? ui_ambient    : u_ambient;
        int loc_spec       = inst ? ui_spec       : u_spec;
        int loc_shiny      = inst ? ui_shiny      : u_shiny;
        int loc_fresnel    = inst ? ui_fresnel    : u_fresnel;
        int loc_ao         = inst ? ui_aoStrength : u_aoStrength;
        int loc_groundY    = inst ? ui_groundY    : u_groundY;
        int loc_light0     = inst ? ui_lightVP0   : u_lightVP0;
        int loc_light1     = inst ? ui_lightVP1   : u_lightVP1;
        int loc_light2     = inst ? ui_lightVP2   : u_lightVP2;
        int loc_shadow0    = inst ? ui_shadowMap0 : u_shadowMap0;
        int loc_shadow1    = inst ? ui_shadowMap1 : u_shadowMap1;
        int loc_shadow2    = inst ? ui_shadowMap2 : u_shadowMap2;
        int loc_sres       = inst ? ui_shadowRes  : u_shadowRes;
        int loc_pcf        = inst ? ui_pcfStep    : u_pcfStep;
        int loc_biasMin    = inst ? ui_biasMin    : u_biasMin;
        int loc_biasMax    = inst ? ui_biasMax    : u_biasMax;
        int loc_splitsLoc  = inst ? ui_splits     : u_splits;
        int loc_transition = inst ? ui_transition : u_transition;
        int loc_exposureL  = inst ? ui_exposure   : u_exposure;

        SetShaderValue(sh, loc_viewPos, viewPosArr, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, loc_sunDir, sunDir, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, loc_sky, sky, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, loc_ground, ground, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, loc_hemi, &hemiStr, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, loc_sun, &sunStr, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, loc_ambient, ambient, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, loc_spec, &specStr, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, loc_shiny, &shininess, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, loc_fresnel, &fresnel, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, loc_ao, &aoStr, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, loc_groundY, &groundY, SHADER_UNIFORM_FLOAT);

        SetShaderValueMatrix(sh, loc_light0, lightVP[0]);
        SetShaderValueMatrix(sh, loc_light1, lightVP[1]);
        SetShaderValueMatrix(sh, loc_light2, lightVP[2]);

        SetShaderValue(sh, loc_splitsLoc, splitVec, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, loc_transition, &transitionFrac, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, loc_exposureL, &exposure, SHADER_UNIFORM_FLOAT);

        // bind sampler slots for this program
        rlSetUniform(loc_shadow0, &slot0, SHADER_UNIFORM_INT, 1);
        rlSetUniform(loc_shadow1, &slot1, SHADER_UNIFORM_INT, 1);
        rlSetUniform(loc_shadow2, &slot2, SHADER_UNIFORM_INT, 1);
    };

    // Apply per-frame values to both non-instanced and instanced shaders
    SetPerFrame(gLitShader, false);
    SetPerFrame(gLitShaderInst, true);

    // --- Separate MeshParts from regular Parts ---
    std::vector<std::shared_ptr<BasePart>> regularParts;
    std::vector<std::shared_ptr<MeshPart>> meshParts;
    
    
    for (auto& p : opaques) {
        // Check if this is a MeshPart
        auto meshPart = std::dynamic_pointer_cast<MeshPart>(p);
        if (meshPart) {
            meshParts.push_back(meshPart);
        } else {
            regularParts.push_back(p);
        }
    }
    
    // --- Regular Parts: batch by color AND shape -> instanced draws ---
    struct BatchKey {
        uint32_t color;
        int shape;
        
        bool operator==(const BatchKey& other) const {
            return color == other.color && shape == other.shape;
        }
    };
    
    struct BatchKeyHash {
        std::size_t operator()(const BatchKey& k) const {
            return std::hash<uint32_t>()(k.color) ^ (std::hash<int>()(k.shape) << 1);
        }
    };
    
    std::unordered_map<BatchKey, std::vector<Matrix>, BatchKeyHash> batches;
    batches.reserve(64);

    auto pack = [](unsigned r,unsigned g,unsigned b,unsigned a)->uint32_t{
        return (r<<24) | (g<<16) | (b<<8) | a;
    };

    for (auto& p : regularParts) {
        Color c = ToRaylibColor(p->Color, 1.0f);
        uint32_t colorKey = pack(c.r,c.g,c.b,c.a);
        BatchKey key = {colorKey, p->Shape};
        batches[key].push_back(BuildInstanceMatrix(p->CF, p->Size));
    }

    // Use instanced material/shader for regular part batches
    if (!batches.empty()) {
        // Ensure instanced material uses our instanced shader
        gPartMatInst.shader = gLitShaderInst;

        for (auto& kv : batches) {
            auto &xforms = kv.second;
            if (xforms.empty()) continue;
            
            BatchKey key = kv.first;
            Color c = { (unsigned char)(key.color>>24), (unsigned char)(key.color>>16), 
                       (unsigned char)(key.color>>8), (unsigned char)(key.color&0xFF) };
            gPartMatInst.maps[MATERIAL_MAP_DIFFUSE].color = c;
            
            // Get or create the appropriate mesh for this shape
            Mesh* meshToUse = nullptr;
            if (gShapeModels.find(key.shape) == gShapeModels.end()) {
                // Create and cache the model for this shape
                Mesh shapeMesh = GenerateMeshForShape(key.shape);
                Model shapeModel = LoadModelFromMesh(shapeMesh);
                shapeModel.materials[0].shader = gLitShaderInst;
                gShapeModels[key.shape] = shapeModel;
            }
            meshToUse = &gShapeModels[key.shape].meshes[0];
            
            DrawMeshInstanced(*meshToUse, gPartMatInst, xforms.data(), (int)xforms.size());
        }
    }

    // --- MeshParts: render individually with custom meshes ---
    for (auto& meshPart : meshParts) {
        Color c = ToRaylibColor(meshPart->Color, 1.0f);
        Vector3 pos = meshPart->CF.p.toRay();
        Vector3 axis; float angleDeg;
        CFrameToAxisAngle(meshPart->CF, axis, angleDeg);
        
        // Check if we have a valid loaded model with meshes
        bool hasValidMesh = meshPart->HasLoadedMesh && 
                           meshPart->LoadedModel.meshCount > 0 && 
                           meshPart->LoadedModel.meshes != nullptr;
        
        if (!hasValidMesh) {
            // Fall back to cube rendering if no mesh is loaded
            BeginShaderMode(gLitShader);
            DrawModelEx(gPartModel, pos, axis, angleDeg, meshPart->Size, c);
            EndShaderMode();
            continue;
        }

        // Render the custom mesh        
        // Calculate proper scale: Size / MeshSize to scale from mesh's original size to desired size
        Vector3 finalSize = {
            meshPart->Size.x / meshPart->MeshSize.x,
            meshPart->Size.y / meshPart->MeshSize.y,
            meshPart->Size.z / meshPart->MeshSize.z
        };
        
        // Apply offset
        Vector3 offsetPos = {
            pos.x + meshPart->Offset.x,
            pos.y + meshPart->Offset.y,
            pos.z + meshPart->Offset.z
        };
        
        // Set the shader and color for the loaded model
        for (int i = 0; i < meshPart->LoadedModel.materialCount; i++) {
            meshPart->LoadedModel.materials[i].shader = gLitShader;
            // Only override color if no texture is loaded, otherwise keep the texture
            if (meshPart->LoadedTexture.id == 0) {
                meshPart->LoadedModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = c;
            } else {
                // Ensure texture is applied and color is white to not tint the texture
                meshPart->LoadedModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = meshPart->LoadedTexture;
                meshPart->LoadedModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
            }
        }
        
        BeginShaderMode(gLitShader);
        // Use WHITE color if texture is loaded to avoid tinting the texture
        Color renderColor = (meshPart->LoadedTexture.id > 0) ? WHITE : c;
        DrawModelEx(meshPart->LoadedModel, offsetPos, axis, angleDeg, finalSize, renderColor);
        EndShaderMode();
    }

    // Transparencies (sorted back-to-front)
    std::sort(transparents.begin(), transparents.end(),
              [](const TItem& A, const TItem& B){ return A.dist2 > B.dist2; });
    BeginBlendMode(BLEND_ALPHA);
    rlDisableDepthMask();
    for (auto& it : transparents) {
        Color c = ToRaylibColor(it.p->Color, it.alpha);
        Vector3 pos = it.p->CF.p.toRay();
        Vector3 axis; float angleDeg;
        CFrameToAxisAngle(it.p->CF, axis, angleDeg);
        DrawModelEx(gPartModel, pos, axis, angleDeg, it.p->Size, c);
    }
    rlEnableDepthMask();
    EndBlendMode();

    if (kCullBackFace) rlDisableBackfaceCulling();

    EndMode3D();
    
    // Debug information (inspired by Raylib example)
    DrawFPS(10,10);
    
    if (kShowShadowDebug) {
        DrawText("Shadow Debug Mode ON", 10, 40, 20, YELLOW);
        DrawText(TextFormat("Shadow Bias Multiplier: %.1f", kShadowBiasMultiplier), 10, 65, 16, WHITE);
        DrawText(TextFormat("Shadow Distance: %.1f", kShadowMaxDistance), 10, 85, 16, WHITE);
        DrawText(TextFormat("Shadow Resolution: %d", kShadowRes), 10, 105, 16, WHITE);
        DrawText(TextFormat("Cascades: %d", kNumCascades), 10, 125, 16, WHITE);
        DrawText("F1: Toggle Debug | F2: Toggle Shadows", 10, 150, 14, GRAY);
        DrawText("[ ]: Decrease Bias | ] : Increase Bias", 10, 170, 14, GRAY);
    } else {
        DrawText("Press F1 for shadow debug info", 10, 40, 14, GRAY);
    }
    
    if (!kEnableShadows) {
        DrawText("SHADOWS DISABLED", GetScreenWidth() - 200, 10, 20, RED);
    }
    
    // Note: EndDrawing() is now called in main.cpp after GUI rendering
}

// ---------------- Optional cleanup ----------------
void ShutdownRendererShadowResources(){
    for (int i=0;i<3;i++){
        if (gShadowMapCSM[i].id) { UnloadShadowmapRenderTexture(gShadowMapCSM[i]); gShadowMapCSM[i] = {0}; }
    }
    if (gSkyModel.meshCount) { UnloadModel(gSkyModel); gSkyModel = {0}; }
    if (gPartModel.meshCount){ UnloadModel(gPartModel); gPartModel = {0}; }
    if (gPartMatInst.shader.id){ UnloadMaterial(gPartMatInst); gPartMatInst = {0}; }
    if (gSkyShader.id) { UnloadShader(gSkyShader); gSkyShader = {0}; }
    if (gLitShaderInst.id){ UnloadShader(gLitShaderInst); gLitShaderInst = {0}; }
    if (gLitShader.id) { UnloadShader(gLitShader); gLitShader = {0}; }
}
