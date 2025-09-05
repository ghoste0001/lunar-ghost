#include "bootstrap/services/TweenService.h"
#include "bootstrap/Game.h"
#include "bootstrap/Instance.h"
#include "bootstrap/instances/BasePart.h"
#include "core/logging/Logging.h"
#include <cstring>
#include <cmath>
#include <algorithm>

extern void Lua_PushSignal(lua_State* L, const std::shared_ptr<RTScriptSignal>& sig);
extern void Lua_PushTween(lua_State* L, const std::shared_ptr<Tween>& tween);

// Forward declarations for Lua bindings
struct LuaTweenInfoUD { TweenInfo info; };

// Math constants
const float PI_CONST = 3.14159265359f;

// Easing functions implementation
float EaseLinear(float t) {
    return t;
}

float EaseSine(float t, EasingDirection direction) {
    switch (direction) {
        case EasingDirection::In:
            return 1.0f - cosf(t * PI_CONST / 2.0f);
        case EasingDirection::Out:
            return sinf(t * PI_CONST / 2.0f);
        case EasingDirection::InOut:
            return -(cosf(PI_CONST * t) - 1.0f) / 2.0f;
    }
    return t;
}

float EaseQuad(float t, EasingDirection direction) {
    switch (direction) {
        case EasingDirection::In:
            return t * t;
        case EasingDirection::Out:
            return 1.0f - (1.0f - t) * (1.0f - t);
        case EasingDirection::InOut:
            return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
    }
    return t;
}

float EaseCubic(float t, EasingDirection direction) {
    switch (direction) {
        case EasingDirection::In:
            return t * t * t;
        case EasingDirection::Out:
            return 1.0f - powf(1.0f - t, 3.0f);
        case EasingDirection::InOut:
            return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
    }
    return t;
}

float EaseQuart(float t, EasingDirection direction) {
    switch (direction) {
        case EasingDirection::In:
            return t * t * t * t;
        case EasingDirection::Out:
            return 1.0f - powf(1.0f - t, 4.0f);
        case EasingDirection::InOut:
            return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 4.0f) / 2.0f;
    }
    return t;
}

float EaseQuint(float t, EasingDirection direction) {
    switch (direction) {
        case EasingDirection::In:
            return t * t * t * t * t;
        case EasingDirection::Out:
            return 1.0f - powf(1.0f - t, 5.0f);
        case EasingDirection::InOut:
            return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 5.0f) / 2.0f;
    }
    return t;
}

float EaseExponential(float t, EasingDirection direction) {
    switch (direction) {
        case EasingDirection::In:
            return t == 0.0f ? 0.0f : powf(2.0f, 10.0f * (t - 1.0f));
        case EasingDirection::Out:
            return t == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
        case EasingDirection::InOut:
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return t < 0.5f ? powf(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - powf(2.0f, -20.0f * t + 10.0f)) / 2.0f;
    }
    return t;
}

float EaseCircular(float t, EasingDirection direction) {
    switch (direction) {
        case EasingDirection::In:
            return 1.0f - sqrtf(1.0f - powf(t, 2.0f));
        case EasingDirection::Out:
            return sqrtf(1.0f - powf(t - 1.0f, 2.0f));
        case EasingDirection::InOut:
            return t < 0.5f ? (1.0f - sqrtf(1.0f - powf(2.0f * t, 2.0f))) / 2.0f : (sqrtf(1.0f - powf(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
    }
    return t;
}

float EaseBack(float t, EasingDirection direction) {
    const float c1 = 1.70158f;
    const float c2 = c1 * 1.525f;
    const float c3 = c1 + 1.0f;

    switch (direction) {
        case EasingDirection::In:
            return c3 * t * t * t - c1 * t * t;
        case EasingDirection::Out:
            return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
        case EasingDirection::InOut:
            return t < 0.5f ? (powf(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f : (powf(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
    }
    return t;
}

float EaseElastic(float t, EasingDirection direction) {
    const float c4 = (2.0f * PI_CONST) / 3.0f;

    switch (direction) {
        case EasingDirection::In:
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return -powf(2.0f, 10.0f * t - 10.0f) * sinf((t * 10.0f - 10.75f) * c4);
        case EasingDirection::Out:
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
        case EasingDirection::InOut:
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            const float c5 = (2.0f * PI_CONST) / 4.5f;
            return t < 0.5f ? -(powf(2.0f, 20.0f * t - 10.0f) * sinf((20.0f * t - 11.125f) * c5)) / 2.0f : (powf(2.0f, -20.0f * t + 10.0f) * sinf((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
    }
    return t;
}

float EaseBounce(float t, EasingDirection direction) {
    auto bounceOut = [](float t) -> float {
        const float n1 = 7.5625f;
        const float d1 = 2.75f;

        if (t < 1.0f / d1) {
            return n1 * t * t;
        } else if (t < 2.0f / d1) {
            return n1 * (t -= 1.5f / d1) * t + 0.75f;
        } else if (t < 2.5f / d1) {
            return n1 * (t -= 2.25f / d1) * t + 0.9375f;
        } else {
            return n1 * (t -= 2.625f / d1) * t + 0.984375f;
        }
    };

    switch (direction) {
        case EasingDirection::In:
            return 1.0f - bounceOut(1.0f - t);
        case EasingDirection::Out:
            return bounceOut(t);
        case EasingDirection::InOut:
            return t < 0.5f ? (1.0f - bounceOut(1.0f - 2.0f * t)) / 2.0f : (1.0f + bounceOut(2.0f * t - 1.0f)) / 2.0f;
    }
    return t;
}

// Tween implementation
Tween::Tween(std::shared_ptr<Instance> instance, const TweenInfo& tweenInfo, 
             const std::unordered_map<std::string, PropertyValue>& properties, TweenService* service)
    : instance(instance), tweenInfo(tweenInfo), targetProperties(properties), tweenService(service) {
    
    LuaScheduler* sch = (g_game && g_game->luaScheduler) ? g_game->luaScheduler.get() : nullptr;
    Completed = std::make_shared<RTScriptSignal>(sch);
    
    // Store initial property values
    for (const auto& [propName, targetValue] : targetProperties) {
        startProperties[propName] = GetProperty(propName);
    }
}

Tween::~Tween() {
    if (tweenService && !isDestroyed) {
        tweenService->RemoveTween(this);
    }
}

void Tween::Play() {
    if (isDestroyed) return;
    
    if (playbackState == PlaybackState::Begin || playbackState == PlaybackState::Paused) {
        if (tweenInfo.DelayTime > 0.0f && playbackState == PlaybackState::Begin) {
            playbackState = PlaybackState::Delayed;
            delayTimer = 0.0f;
        } else {
            playbackState = PlaybackState::Playing;
        }
    }
}

void Tween::Pause() {
    if (isDestroyed) return;
    
    if (playbackState == PlaybackState::Playing || playbackState == PlaybackState::Delayed) {
        playbackState = PlaybackState::Paused;
    }
}

void Tween::Cancel() {
    if (isDestroyed) return;
    
    playbackState = PlaybackState::Cancelled;
    FireCompleted();
}

void Tween::Destroy() {
    if (isDestroyed) return;
    
    isDestroyed = true;
    playbackState = PlaybackState::Cancelled;
    
    if (tweenService) {
        tweenService->RemoveTween(this);
    }
}

void Tween::Update(float deltaTime) {
    if (isDestroyed || playbackState == PlaybackState::Completed || playbackState == PlaybackState::Cancelled) {
        return;
    }
    
    auto inst = instance.lock();
    if (!inst) {
        Cancel();
        return;
    }
    
    // Handle delay
    if (playbackState == PlaybackState::Delayed) {
        delayTimer += deltaTime;
        if (delayTimer >= tweenInfo.DelayTime) {
            playbackState = PlaybackState::Playing;
        } else {
            return;
        }
    }
    
    if (playbackState != PlaybackState::Playing) {
        return;
    }
    
    currentTime += deltaTime;
    float progress = std::min(currentTime / tweenInfo.Time, 1.0f);
    
    // Apply easing
    float easedProgress = EaseValue(progress, tweenInfo.EasingStyle, tweenInfo.EasingDirection);
    
    // Handle reversing
    if (isReversing) {
        easedProgress = 1.0f - easedProgress;
    }
    
    // Interpolate and set properties
    for (const auto& [propName, targetValue] : targetProperties) {
        const PropertyValue& startValue = startProperties[propName];
        PropertyValue currentValue = LerpProperty(startValue, targetValue, easedProgress);
        SetProperty(propName, currentValue);
    }
    
    // Check if tween is complete
    if (progress >= 1.0f) {
        if (tweenInfo.Reverses && !isReversing) {
            // Start reversing
            isReversing = true;
            currentTime = 0.0f;
        } else if (currentRepeat < tweenInfo.RepeatCount) {
            // Repeat
            currentRepeat++;
            currentTime = 0.0f;
            isReversing = false;
        } else {
            // Complete
            playbackState = PlaybackState::Completed;
            FireCompleted();
        }
    }
}

float Tween::EaseValue(float t, EasingStyle style, EasingDirection direction) {
    switch (style) {
        case EasingStyle::Linear:
            return EaseLinear(t);
        case EasingStyle::Sine:
            return EaseSine(t, direction);
        case EasingStyle::Quad:
            return EaseQuad(t, direction);
        case EasingStyle::Cubic:
            return EaseCubic(t, direction);
        case EasingStyle::Quart:
            return EaseQuart(t, direction);
        case EasingStyle::Quint:
            return EaseQuint(t, direction);
        case EasingStyle::Exponential:
            return EaseExponential(t, direction);
        case EasingStyle::Circular:
            return EaseCircular(t, direction);
        case EasingStyle::Back:
            return EaseBack(t, direction);
        case EasingStyle::Elastic:
            return EaseElastic(t, direction);
        case EasingStyle::Bounce:
            return EaseBounce(t, direction);
        default:
            return t;
    }
}

PropertyValue Tween::LerpProperty(const PropertyValue& start, const PropertyValue& target, float alpha) {
    // Both values must be the same type
    if (start.index() != target.index()) {
        return start; // Return start value if types don't match
    }
    
    // Handle each type explicitly to avoid template deduction issues
    if (std::holds_alternative<float>(start)) {
        const float& startVal = std::get<float>(start);
        const float& targetVal = std::get<float>(target);
        return startVal + (targetVal - startVal) * alpha;
    } else if (std::holds_alternative<Vector3Game>(start)) {
        const Vector3Game& startVal = std::get<Vector3Game>(start);
        const Vector3Game& targetVal = std::get<Vector3Game>(target);
        return startVal.lerp(targetVal, alpha);
    } else if (std::holds_alternative<Color3>(start)) {
        const Color3& startVal = std::get<Color3>(start);
        const Color3& targetVal = std::get<Color3>(target);
        return startVal.lerp(targetVal, alpha);
    } else {
        return start; // Fallback for unsupported types
    }
}

void Tween::SetProperty(const std::string& propertyName, const PropertyValue& value) {
    auto inst = instance.lock();
    if (!inst) return;
    
    // Try to cast to BasePart first for direct property access
    auto basePart = std::dynamic_pointer_cast<BasePart>(inst);
    if (basePart) {
        if (propertyName == "Transparency") {
            if (std::holds_alternative<float>(value)) {
                basePart->Transparency = std::get<float>(value);
            }
            return;
        } else if (propertyName == "Reflectance") {
            if (std::holds_alternative<float>(value)) {
                basePart->Reflectance = std::get<float>(value);
            }
            return;
        } else if (propertyName == "Density") {
            if (std::holds_alternative<float>(value)) {
                basePart->Density = std::get<float>(value);
            }
            return;
        } else if (propertyName == "Friction") {
            if (std::holds_alternative<float>(value)) {
                basePart->Friction = std::get<float>(value);
            }
            return;
        } else if (propertyName == "Elasticity") {
            if (std::holds_alternative<float>(value)) {
                basePart->Elasticity = std::get<float>(value);
            }
            return;
        } else if (propertyName == "Position") {
            if (std::holds_alternative<Vector3Game>(value)) {
                basePart->CF.p = std::get<Vector3Game>(value);
            }
            return;
        } else if (propertyName == "Size") {
            if (std::holds_alternative<Vector3Game>(value)) {
                basePart->Size = std::get<Vector3Game>(value).toRay();
            }
            return;
        } else if (propertyName == "Color") {
            if (std::holds_alternative<Color3>(value)) {
                basePart->Color = std::get<Color3>(value);
            }
            return;
        }
    }
    
    // Fallback to generic attribute setting for other instances or unknown properties
    std::visit([&inst, &propertyName](const auto& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, float>) {
            inst->SetAttribute(propertyName, static_cast<double>(val));
        }
        // For Vector3Game and Color3, we'd need more complex attribute handling
    }, value);
}

PropertyValue Tween::GetProperty(const std::string& propertyName) {
    auto inst = instance.lock();
    if (!inst) return 0.0f;
    
    // Try to cast to BasePart first for direct property access
    auto basePart = std::dynamic_pointer_cast<BasePart>(inst);
    if (basePart) {
        if (propertyName == "Transparency") {
            return basePart->Transparency;
        } else if (propertyName == "Reflectance") {
            return basePart->Reflectance;
        } else if (propertyName == "Density") {
            return basePart->Density;
        } else if (propertyName == "Friction") {
            return basePart->Friction;
        } else if (propertyName == "Elasticity") {
            return basePart->Elasticity;
        } else if (propertyName == "Position") {
            return basePart->CF.p;
        } else if (propertyName == "Size") {
            return Vector3Game::fromRay(basePart->Size);
        } else if (propertyName == "Color") {
            return basePart->Color;
        }
    }
    
    // Fallback to generic attribute getting
    auto attr = inst->GetAttribute(propertyName);
    if (attr.has_value()) {
        if (std::holds_alternative<double>(*attr)) {
            return static_cast<float>(std::get<double>(*attr));
        }
    }
    
    return 0.0f; // Default value
}

void Tween::FireCompleted() {
    if (Completed && !Completed->IsClosed()) {
        // Fire the signal with no arguments
        if (g_game && g_game->luaScheduler) {
            lua_State* L = g_game->luaScheduler->GetMainState();
            if (L) {
                Completed->Fire(L, 0, 0);  // No arguments
            }
        }
    }
}

// TweenService implementation
TweenService::TweenService() : Service("TweenService", InstanceClass::Unknown) {
}

TweenService::~TweenService() {
    // Clean up active tweens
    for (auto& tween : activeTweens) {
        if (tween) {
            tween->Destroy();
        }
    }
    activeTweens.clear();
}

std::shared_ptr<Tween> TweenService::Create(std::shared_ptr<Instance> instance, const TweenInfo& tweenInfo,
                                            const std::unordered_map<std::string, PropertyValue>& properties) {
    if (!instance) {
        return nullptr;
    }
    
    auto tween = std::make_shared<Tween>(instance, tweenInfo, properties, this);
    activeTweens.push_back(tween);
    
    return tween;
}

float TweenService::GetValue(float alpha, EasingStyle easingStyle, EasingDirection easingDirection) {
    // Use the static easing functions directly
    switch (easingStyle) {
        case EasingStyle::Linear:
            return EaseLinear(alpha);
        case EasingStyle::Sine:
            return EaseSine(alpha, easingDirection);
        case EasingStyle::Quad:
            return EaseQuad(alpha, easingDirection);
        case EasingStyle::Cubic:
            return EaseCubic(alpha, easingDirection);
        case EasingStyle::Quart:
            return EaseQuart(alpha, easingDirection);
        case EasingStyle::Quint:
            return EaseQuint(alpha, easingDirection);
        case EasingStyle::Exponential:
            return EaseExponential(alpha, easingDirection);
        case EasingStyle::Circular:
            return EaseCircular(alpha, easingDirection);
        case EasingStyle::Back:
            return EaseBack(alpha, easingDirection);
        case EasingStyle::Elastic:
            return EaseElastic(alpha, easingDirection);
        case EasingStyle::Bounce:
            return EaseBounce(alpha, easingDirection);
        default:
            return alpha;
    }
}

void TweenService::Update(float deltaTime) {
    // Update all active tweens
    for (auto it = activeTweens.begin(); it != activeTweens.end();) {
        auto& tween = *it;
        if (!tween || tween->GetPlaybackState() == PlaybackState::Completed || 
            tween->GetPlaybackState() == PlaybackState::Cancelled) {
            it = activeTweens.erase(it);
        } else {
            tween->Update(deltaTime);
            ++it;
        }
    }
}

void TweenService::RemoveTween(Tween* tween) {
    activeTweens.erase(
        std::remove_if(activeTweens.begin(), activeTweens.end(),
                      [tween](const std::shared_ptr<Tween>& sharedTween) {
                          return !sharedTween || sharedTween.get() == tween;
                      }),
        activeTweens.end());
}

// Static C functions for Lua bindings
static int l_TweenService_Create(lua_State* L) {
    // TweenService:Create(instance, tweenInfo, properties)
    auto* inst_ptr = static_cast<std::shared_ptr<Instance>*>(luaL_checkudata(L, 2, "Librebox.Instance"));
    if (!inst_ptr || !*inst_ptr) {
        lua_pushnil(L);
        return 1;
    }
    
    auto* tweenInfoUD = static_cast<LuaTweenInfoUD*>(luaL_checkudata(L, 3, "Librebox.TweenInfo"));
    if (!tweenInfoUD) {
        luaL_error(L, "TweenService:Create expects TweenInfo as second argument");
        return 0;
    }
    
    luaL_checktype(L, 4, LUA_TTABLE);
    
    // Parse properties table
    std::unordered_map<std::string, PropertyValue> properties;
    lua_pushnil(L);
    while (lua_next(L, 4) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING) {
            const char* propName = lua_tostring(L, -2);
            
            // Check the type of the value and convert accordingly
            if (lua_type(L, -1) == LUA_TNUMBER) {
                float propValue = (float)lua_tonumber(L, -1);
                properties[propName] = propValue;
            } else if (lua_type(L, -1) == LUA_TUSERDATA) {
                // Check if it's a Vector3Game
                if (auto* vec3 = lb::check<Vector3Game>(L, -1)) {
                    properties[propName] = *vec3;
                }
                // Check if it's a Color3
                else if (auto* color = lb::check<Color3>(L, -1)) {
                    properties[propName] = *color;
                }
            } else if (lua_type(L, -1) == LUA_TTABLE) {
                // Handle Vector3 and Color3 tables created by Vector3.new() and Color3.fromRGB()
                
                // Check if it's a Vector3 table (has X, Y, Z fields)
                lua_getfield(L, -1, "X");
                lua_getfield(L, -2, "Y");
                lua_getfield(L, -3, "Z");
                
                if (lua_isnumber(L, -3) && lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
                    float x = (float)lua_tonumber(L, -3);
                    float y = (float)lua_tonumber(L, -2);
                    float z = (float)lua_tonumber(L, -1);
                    properties[propName] = Vector3Game(x, y, z);
                    lua_pop(L, 3); // Pop X, Y, Z values
                } else {
                    lua_pop(L, 3); // Pop X, Y, Z values
                    
                    // Check if it's a Color3 table (has R, G, B fields)
                    lua_getfield(L, -1, "R");
                    lua_getfield(L, -2, "G");
                    lua_getfield(L, -3, "B");
                    
                    if (lua_isnumber(L, -3) && lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
                        float r = (float)lua_tonumber(L, -3);
                        float g = (float)lua_tonumber(L, -2);
                        float b = (float)lua_tonumber(L, -1);
                        properties[propName] = Color3(r, g, b);
                    }
                    lua_pop(L, 3); // Pop R, G, B values
                }
            }
        }
        lua_pop(L, 1);
    }
    
    // Get TweenService instance
    auto tweenService = std::dynamic_pointer_cast<TweenService>(Service::Get("TweenService"));
    if (!tweenService) {
        lua_pushnil(L);
        return 1;
    }
    
    // Create tween
    auto tween = tweenService->Create(*inst_ptr, tweenInfoUD->info, properties);
    if (!tween) {
        lua_pushnil(L);
        return 1;
    }
    
    // Push tween to Lua
    Lua_PushTween(L, tween);
    return 1;
}

static int l_TweenService_GetValue(lua_State* L) {
    // TweenService:GetValue(alpha, easingStyle, easingDirection)
    float alpha = (float)luaL_checknumber(L, 2);
    int easingStyleInt = (int)luaL_checkinteger(L, 3);
    int easingDirectionInt = (int)luaL_checkinteger(L, 4);
    
    EasingStyle easingStyle = static_cast<EasingStyle>(easingStyleInt);
    EasingDirection easingDirection = static_cast<EasingDirection>(easingDirectionInt);
    
    auto tweenService = std::dynamic_pointer_cast<TweenService>(Service::Get("TweenService"));
    if (!tweenService) {
        lua_pushnumber(L, alpha);
        return 1;
    }
    
    float result = tweenService->GetValue(alpha, easingStyle, easingDirection);
    lua_pushnumber(L, result);
    return 1;
}

bool TweenService::LuaGet(lua_State* L, const char* key) const {
    if (std::strcmp(key, "Create") == 0) {
        lua_pushcfunction(L, l_TweenService_Create, "Create");
        return true;
    }
    
    if (std::strcmp(key, "GetValue") == 0) {
        lua_pushcfunction(L, l_TweenService_GetValue, "GetValue");
        return true;
    }
    
    return false;
}

// Register TweenService
static Instance::Registrar s_regTweenService("TweenService", []{
    return std::make_shared<TweenService>();
});
