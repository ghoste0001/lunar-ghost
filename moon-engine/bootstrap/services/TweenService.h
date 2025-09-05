#pragma once
#include "bootstrap/services/Service.h"
#include "bootstrap/signals/Signal.h"
#include "core/datatypes/Vector3Game.h"
#include "core/datatypes/Color3.h"
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <variant>
#include <raylib.h>

struct lua_State;

// Easing styles matching Roblox API
enum class EasingStyle {
    Linear,
    Sine,
    Back,
    Quad,
    Quart,
    Quint,
    Bounce,
    Elastic,
    Exponential,
    Circular,
    Cubic
};

// Easing directions matching Roblox API
enum class EasingDirection {
    In,
    Out,
    InOut
};

// TweenInfo structure matching Roblox API
struct TweenInfo {
    float Time = 1.0f;
    ::EasingStyle EasingStyle = ::EasingStyle::Quad;
    ::EasingDirection EasingDirection = ::EasingDirection::Out;
    int RepeatCount = 0;
    bool Reverses = false;
    float DelayTime = 0.0f;

    TweenInfo() = default;
    TweenInfo(float time, ::EasingStyle style = ::EasingStyle::Quad, ::EasingDirection direction = ::EasingDirection::Out,
              int repeatCount = 0, bool reverses = false, float delayTime = 0.0f)
        : Time(time), EasingStyle(style), EasingDirection(direction), 
          RepeatCount(repeatCount), Reverses(reverses), DelayTime(delayTime) {}
};

// Tween states matching Roblox API
enum class PlaybackState {
    Begin,
    Delayed,
    Playing,
    Paused,
    Completed,
    Cancelled
};

// Property value type that can hold different types
using PropertyValue = std::variant<float, Vector3Game, Color3>;

// Forward declaration
class TweenService;

// Tween class matching Roblox API
class Tween {
public:
    std::shared_ptr<RTScriptSignal> Completed;
    
    Tween(std::shared_ptr<Instance> instance, const TweenInfo& tweenInfo, 
          const std::unordered_map<std::string, PropertyValue>& properties, TweenService* service);
    ~Tween();

    // Tween methods
    void Play();
    void Pause();
    void Cancel();
    void Destroy();

    // Properties
    PlaybackState GetPlaybackState() const { return playbackState; }
    TweenInfo GetTweenInfo() const { return tweenInfo; }
    std::shared_ptr<Instance> GetInstance() const { return instance.lock(); }

    // Internal update method
    void Update(float deltaTime);

    // Public easing function for TweenService::GetValue
    float EaseValue(float t, EasingStyle style, EasingDirection direction);

private:
    std::weak_ptr<Instance> instance;
    TweenInfo tweenInfo;
    std::unordered_map<std::string, PropertyValue> targetProperties;
    std::unordered_map<std::string, PropertyValue> startProperties;
    TweenService* tweenService;
    
    PlaybackState playbackState = PlaybackState::Begin;
    float currentTime = 0.0f;
    float delayTimer = 0.0f;
    int currentRepeat = 0;
    bool isReversing = false;
    bool isDestroyed = false;

    void SetProperty(const std::string& propertyName, const PropertyValue& value);
    PropertyValue GetProperty(const std::string& propertyName);
    PropertyValue LerpProperty(const PropertyValue& start, const PropertyValue& target, float alpha);
    void FireCompleted();
};

// TweenService class matching Roblox API
struct TweenService : Service {
    TweenService();
    ~TweenService();

    // TweenService methods
    std::shared_ptr<Tween> Create(std::shared_ptr<Instance> instance, const TweenInfo& tweenInfo,
                                  const std::unordered_map<std::string, PropertyValue>& properties);
    
    float GetValue(float alpha, EasingStyle easingStyle, EasingDirection easingDirection);

    // Update all active tweens
    void Update(float deltaTime);

    // Lua bindings
    bool LuaGet(lua_State* L, const char* key) const override;

private:
    std::vector<std::shared_ptr<Tween>> activeTweens;
    
    void RemoveTween(Tween* tween);
    friend class Tween;
};
