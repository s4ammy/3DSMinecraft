#pragma once

namespace ControlSettings {
    enum class EMode : unsigned char {
        L_CIRCLE_PAD,
        CIRCLE_PAD_PRO,
        TOUCH_STICK,
        ABXY_CAMERA,
        COUNT
    };

    enum class EContext : unsigned char {
        GAMEPLAY,
        MENU
    };

    enum class ESetting : unsigned char {
        LOOK_X,
        LOOK_Y,
        MODIFIER_TOGGLE,
        TOUCH_X,
        TOUCH_Y,
        TOUCH_RADIUS,
        TOUCH_DEAD_ZONE,
        TOUCH_OPACITY,
        TOUCH_HIDE_IDLE,
        OVERLAY,
        PARTICLES,
        WEATHER,
        MAP_RATE,
        LOW_MEMORY_TEXTURES,
        CONFLICT_HINTS,
        OVERLAY_POSITION,
        OVERLAY_SCALE,
        OVERLAY_OPACITY,
        CAMERA_CURVE,
        CAMERA_ACCELERATION,
        FRAME_GRAPH,
        RENDER_DISTANCE,
        CHUNK_RADIUS,
        MAP_RADIUS,
        MESH_BUILDERS,
        COUNT
    };

    inline constexpr unsigned int modeCount = 4;
    inline constexpr unsigned int legacyModeCount = 3;
    inline constexpr unsigned int gameplayBindingCount = 15;
    inline constexpr unsigned int menuBindingCount = 14;
    inline constexpr unsigned int cameraBindingBegin = gameplayBindingCount + menuBindingCount;
    inline constexpr unsigned int bindingCount = cameraBindingBegin + 4;
    inline constexpr unsigned int bindableButtons = 0xcfff;
    inline constexpr unsigned int cameraModifierIndex = 14;
    inline constexpr unsigned int modeSettingCount = static_cast<unsigned int>(ESetting::OVERLAY);
    inline constexpr unsigned int globalSettingCount = static_cast<unsigned int>(ESetting::COUNT) - modeSettingCount;
    inline constexpr unsigned int legacyBindingCount = 27;
    inline constexpr unsigned int legacyEncodedSize = 8 + legacyModeCount * legacyBindingCount * 4;
    inline constexpr unsigned int versionTwoEncodedSize = 8 + modeCount * bindingCount * 4 + (modeCount * modeSettingCount + 6) * 4;
    inline constexpr unsigned int versionThreeEncodedSize = 8 + modeCount * bindingCount * 4 + (modeCount * modeSettingCount + 12) * 4;
    inline constexpr unsigned int encodedSize = 8 + modeCount * bindingCount * 4 + (modeCount * modeSettingCount + globalSettingCount) * 4;

    struct TProfile {
        unsigned short bindings[bindingCount];
    };

    struct TConfig {
        EMode mode;
        TProfile profiles[modeCount];
        unsigned short modeSettings[modeCount][modeSettingCount];
        unsigned short globalSettings[globalSettingCount];
    };

    struct TInputState {
        unsigned int suppressed;
        unsigned int chordConsumed;
        unsigned int previousButtons;
        EContext context;
        EMode mode;
        bool initialized;
        bool cameraLatched;
    };

    struct TSample {
        short moveX;
        short moveY;
        short lookX;
        short lookY;
        unsigned int buttons;
    };

    static_assert(sizeof(TConfig) == 370, "Controls profile layout changed");
    static_assert(sizeof(TInputState) == 16, "Controls input state layout changed");
    static_assert(sizeof(TSample) == 12, "Native input sample layout changed");

    const char *ModeName(EMode mode);
    const char *ActionName(unsigned int index);
    bool IsGameplayBinding(unsigned int index);
    void ResetProfile(TProfile &profile, EMode mode);
    void ResetMode(TConfig &config);
    bool ResetBinding(TConfig &config, unsigned int index);
    bool CopyBindings(TConfig &config, EMode source);
    bool IsShared(const TConfig &config, unsigned int index);
    const char *SettingName(ESetting setting);
    unsigned int GetSetting(const TConfig &config, ESetting setting);
    bool SetSetting(TConfig &config, ESetting setting, unsigned int value);
    void CycleSetting(TConfig &config, ESetting setting);
    void ScaleLook(const TConfig &config, TSample &sample);
    unsigned int PerformancePreset(const TConfig &config);
    void ApplyPerformancePreset(TConfig &config, unsigned int preset);
    void Initialize(TConfig &config);
    bool IsValidBinding(unsigned int buttons);
    bool SetBinding(TConfig &config, unsigned int index, unsigned int buttons);
    void Apply(const TConfig &config, TInputState &state, TSample &sample, EContext context);
    void Encode(const TConfig &config, char *text);
    bool Decode(TConfig &config, const char *text, unsigned int size);
    void DescribeBinding(unsigned int buttons, char *text);
}
