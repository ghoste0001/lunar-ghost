#pragma once

// ---------------- GLSL Version ----------------
#define GLSL_VERSION 330

// Lit + CSM shadows (PCF), hemispheric, spec, fresnel, AO
inline const char* LIT_VS = R"(

#version 330
in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 lightVP0;
uniform mat4 lightVP1;
uniform mat4 lightVP2;

uniform float normalBiasWS0;
uniform float normalBiasWS1;
uniform float normalBiasWS2;

out vec3 vN;
out vec3 vWPos;
out vec2 vTexCoord;
out vec4 vLS0;
out vec4 vLS1;
out vec4 vLS2;

void main(){
    mat3 nmat = mat3(transpose(inverse(matModel)));
    vN = normalize(nmat * vertexNormal);
    vec4 worldPos = matModel * vec4(vertexPosition,1.0);
    vWPos = worldPos.xyz;
    vTexCoord = vertexTexCoord;

    // apply normal-space small offset per-cascade to avoid contact gaps
    vec3 Nw = normalize(vN);
    vec4 worldPos0 = worldPos + vec4(Nw * normalBiasWS0, 0.0);
    vec4 worldPos1 = worldPos + vec4(Nw * normalBiasWS1, 0.0);
    vec4 worldPos2 = worldPos + vec4(Nw * normalBiasWS2, 0.0);

    vLS0 = lightVP0 * worldPos0;
    vLS1 = lightVP1 * worldPos1;
    vLS2 = lightVP2 * worldPos2;

    gl_Position = mvp * vec4(vertexPosition,1.0);
}

)";

inline const char* LIT_VS_INST = R"(

#version 330
in vec3 vertexPosition;
in vec3 vertexNormal;

// Per-instance model matrix provided as vertex attribute
in mat4 instanceTransform;

uniform mat4 mvp;
uniform mat4 lightVP0;
uniform mat4 lightVP1;
uniform mat4 lightVP2;

uniform float normalBiasWS0;
uniform float normalBiasWS1;
uniform float normalBiasWS2;

out vec3 vN;
out vec3 vWPos;
out vec4 vLS0;
out vec4 vLS1;
out vec4 vLS2;

void main(){
    mat3 nmat = mat3(transpose(inverse(instanceTransform)));
    vN = normalize(nmat * vertexNormal);

    vec4 worldPos = instanceTransform * vec4(vertexPosition,1.0);
    vWPos = worldPos.xyz;

    vec3 Nw = normalize(vN);
    vec4 worldPos0 = worldPos + vec4(Nw * normalBiasWS0, 0.0);
    vec4 worldPos1 = worldPos + vec4(Nw * normalBiasWS1, 0.0);
    vec4 worldPos2 = worldPos + vec4(Nw * normalBiasWS2, 0.0);

    vLS0 = lightVP0 * worldPos0;
    vLS1 = lightVP1 * worldPos1;
    vLS2 = lightVP2 * worldPos2;

    gl_Position = mvp * worldPos;
}
    
)";

inline const char* LIT_FS = R"(

#version 330

in vec3 vN;
in vec3 vWPos;
in vec2 vTexCoord;
in vec4 vLS0;
in vec4 vLS1;
in vec4 vLS2;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 NormalColor; // For Normal G-Buffer

uniform vec3 viewPos;

// Lighting
uniform vec3 sunDir;
uniform vec3 skyColor;
uniform vec3 groundColor;
uniform float hemiStrength;
uniform float sunStrength;
uniform vec3 ambientColor;

// Spec/Fresnel
uniform float specStrength;
uniform float shininess;
uniform float fresnelStrength;

// AO
uniform float aoStrength;
uniform float groundY;

// CSM
uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform int shadowMapResolution;
uniform float pcfStep;
uniform float biasMin;
uniform float biasMax;
uniform vec3 cascadeSplits;
uniform float transitionFrac;

// exposure
uniform float exposure;

// Material
uniform vec4 colDiffuse;
uniform sampler2D texture0;

// -----------------------------------------
float SampleShadow(sampler2DShadow smap, vec3 proj, float ndl){
    float bias = mix(biasMax, biasMin, ndl);
    float step = pcfStep / float(shadowMapResolution);
    float sum = 0.0;
    for(int x=-1;x<=1;x++)
    for(int y=-1;y<=1;y++){
        vec2 off = vec2(x,y) * step;
        sum += texture(smap, vec3(proj.xy + off, proj.z - bias));
    }
    return sum / 9.0;
}

float ShadowForCascade(int idx, vec3 proj, float ndl){
    if(proj.x<0.0||proj.x>1.0||proj.y<0.0||proj.y>1.0||proj.z<0.0||proj.z>1.0)
        return 1.0;
    if(idx==0) return SampleShadow(shadowMap0, proj, ndl);
    if(idx==1) return SampleShadow(shadowMap1, proj, ndl);
    return SampleShadow(shadowMap2, proj, ndl);
}

float ShadowBlend(vec3 p0, vec3 p1, vec3 p2, float ndl, float viewDepth, vec3 splits){
    float d0 = splits.x;
    float d1 = splits.y;
    float frac = clamp(transitionFrac,0.0,0.5);
    float band0 = max(0.001,d0*frac);
    float band1 = max(0.001,d1*frac);

    if(viewDepth <= d0 - band0) return ShadowForCascade(0,p0,ndl);
    if(viewDepth < d0 + band0){
        float t = smoothstep(d0-band0,d0+band0,viewDepth);
        return mix(ShadowForCascade(0,p0,ndl), ShadowForCascade(1,p1,ndl), t);
    }
    if(viewDepth <= d1 - band1) return ShadowForCascade(1,p1,ndl);
    if(viewDepth < d1 + band1){
        float t = smoothstep(d1-band1,d1+band1,viewDepth);
        return mix(ShadowForCascade(1,p1,ndl), ShadowForCascade(2,p2,ndl), t);
    }
    return ShadowForCascade(2,p2,ndl);
}

// -----------------------------------------
void main(){
    vec3 N = normalize(vN);
    vec3 L = normalize(-sunDir);
    vec3 V = normalize(viewPos - vWPos);
    vec3 H = normalize(L+V);

    // Hemispheric lighting
    float t = clamp(N.y*0.5 + 0.5, 0.0, 1.0);
    vec3 hemi = mix(groundColor, skyColor, t) * hemiStrength;

    // Diffuse & Specular
    float ndl = max(dot(N,L),0.0);
    float spec = pow(max(dot(N,H),0.0), shininess) * specStrength;
    float FH = pow(1.0 - max(dot(N,V),0.0), 5.0);
    float fresnel = mix(0.02,1.0,FH) * fresnelStrength;

    // AO
    float height = max(vWPos.y - groundY, 0.0);
    float aoGround = clamp(1.0 - exp(-height*0.15),0.3,1.0);
    float ao = mix(1.0, aoGround, aoStrength);

    // Shadow
    float viewDepth = length(viewPos - vWPos);
    vec3 p0 = vLS0.xyz / vLS0.w * 0.5 + 0.5;
    vec3 p1 = vLS1.xyz / vLS1.w * 0.5 + 0.5;
    vec3 p2 = vLS2.xyz / vLS2.w * 0.5 + 0.5;
    float shadow = ShadowBlend(p0,p1,p2,ndl,viewDepth,cascadeSplits);

    // Texture
    vec4 texColor = texture(texture0,vTexCoord);
    vec3 base = pow((texColor * colDiffuse).rgb, vec3(2.2));

    // Lighting
    float sunTerm = sunStrength * ndl * shadow;
    spec *= shadow;
    fresnel *= shadow;
    vec3 color = base*(hemi*ao + sunTerm) + spec + fresnel;
    color += base*ambientColor;
    
    // Note: exposure/tonemapping is now done in the final composite shader
    // We output linear, high-dynamic-range color here.
    FragColor = vec4(color, colDiffuse.a);

    // Output world-space normal to the second render target
    NormalColor = vec4(N * 0.5 + 0.5, 1.0); // Encode to [0,1] range
}
    
)";

// Vertex shader for post-processing fullscreen quad
inline const char* POST_VS = R"(
#version 330
out vec2 TexCoords;
void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    TexCoords = vec2(x*0.5+0.5, y*0.5+0.5);
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)";

// SSAO fragment shader
inline const char* SSAO_FS = R"(
#version 330
in vec2 TexCoords;
out float FragColor;

uniform sampler2D gDepth;
uniform sampler2D gNormal;
uniform sampler2D texNoise;
uniform vec3 samples[64];
uniform mat4 matProjection;
uniform vec2 noiseScale;

uniform float radius;
uniform float bias;
uniform float strength;

void main() {
    // Reconstruct view-space position from depth
    float depth = texture(gDepth, TexCoords).r;
    vec2 screenPos = TexCoords * 2.0 - 1.0;
    vec4 viewRay = inverse(matProjection) * vec4(screenPos, 1.0, 1.0);
    vec3 V = normalize(viewRay.xyz) * (1.0/viewRay.w);
    vec3 P = V * depth;

    // Get view-space normal
    vec3 N = normalize(texture(gNormal, TexCoords).rgb * 2.0 - 1.0);

    // Get random vector
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

    // Create TBN matrix
    vec3 tangent = normalize(randomVec - N * dot(randomVec, N));
    vec3 bitangent = cross(N, tangent);
    mat3 TBN = mat3(tangent, bitangent, N);

    float occlusion = 0.0;
    for(int i = 0; i < 64; ++i) {
        // Get sample position
        vec3 samplePos = TBN * samples[i];
        samplePos = P + samplePos * radius;

        // Project sample position
        vec4 offset = vec4(samplePos, 1.0);
        offset = matProjection * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        
        // Get sample depth
        float sampleDepth = texture(gDepth, offset.xy).r;
        
        // Accumulate occlusion
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(P.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / 64.0);
    FragColor = pow(occlusion, strength);
}
)";

// Simple Box Blur shader
inline const char* BLUR_FS = R"(
#version 330
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D image;
uniform vec2 texSize;

void main() {
    vec2 texelSize = 1.0 / texSize;
    vec4 result = vec4(0.0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            result += texture(image, TexCoords + vec2(x, y) * texelSize);
        }
    }
    FragColor = result / 9.0;
}
)";

// Bloom: extract bright parts of the image
inline const char* BLOOM_EXTRACT_FS = R"(
#version 330
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D sceneTex;
uniform float threshold;

void main() {
    vec3 color = texture(sceneTex, TexCoords).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > threshold) {
        FragColor = vec4(color, 1.0);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
)";


// Final Composite shader: combines everything
inline const char* COMPOSITE_FS = R"(
#version 330
in vec2 TexCoords;
out vec4 FragColor;

// Input textures
uniform sampler2D sceneTex;
uniform sampler2D aoTex;
uniform sampler2D bloomTex;
uniform sampler2D depthTex;

// General
uniform float exposure;

// SSAO
uniform bool enableSSAO;

// Bloom
uniform bool enableBloom;

// Fog
uniform bool enableFog;
uniform vec3 fogColor;
uniform float fogDensity;

// Volumetric Lighting (God Rays)
uniform bool enableVolumetric;
uniform vec2 sunScreenPos;
uniform float volumetricStrength;
const int NUM_SAMPLES = 64;

void main() {
    // --- 1. Get Base Scene Color and AO ---
    vec3 sceneColor = texture(sceneTex, TexCoords).rgb;
    float ao = enableSSAO ? texture(aoTex, TexCoords).r : 1.0;
    vec3 finalColor = sceneColor * ao;

    // --- 2. Add Bloom ---
    if(enableBloom) {
        vec3 bloomColor = texture(bloomTex, TexCoords).rgb;
        finalColor += bloomColor;
    }

    // --- 3. Add Volumetric Lighting ---
    if(enableVolumetric) {
        float depth = texture(depthTex, TexCoords).r;
        if(depth < 1.0) { // Don't apply to sky
            vec2 delta = sunScreenPos - TexCoords;
            delta *= 1.0 / float(NUM_SAMPLES);
            float illumination = 0.0;
            
            for(int i=0; i < NUM_SAMPLES; i++) {
                vec2 sampleCoord = TexCoords + float(i) * delta;
                float sampleDepth = texture(depthTex, sampleCoord).r;
                if(sampleDepth >= 1.0) { // Ray is not occluded by geometry, hits sky
                    illumination += 1.0;
                }
            }
            illumination /= float(NUM_SAMPLES);
            finalColor += vec3(1.0, 0.9, 0.8) * illumination * volumetricStrength;
        }
    }
    
    // --- 4. Add Fog ---
    if(enableFog) {
        float depth = texture(depthTex, TexCoords).r;
        if (depth < 1.0) { // Don't apply to skybox
            float fogFactor = exp2(-fogDensity * fogDensity * depth * depth * 1.442695);
            fogFactor = clamp(fogFactor, 0.0, 1.0);
            finalColor = mix(fogColor, finalColor, fogFactor);
        }
    }

    // --- 5. Tone Mapping & Gamma Correction ---
    // Reinhard tone mapping
    vec3 mapped = finalColor / (finalColor + vec3(1.0));
    // Apply exposure
    mapped *= exposure;
    // Gamma correction
    mapped = pow(mapped, vec3(1.0/2.2));

    FragColor = vec4(mapped, 1.0);
}
)";