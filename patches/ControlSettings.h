#pragma once

namespace ControlSettings {
    enum class EMode : unsigned char {
        L_CIRCLE_PAD,
        CIRCLE_PAD_PRO,
        TOUCH_STICK,
        COUNT
    };

    enum class EContext : unsigned char {
        GAMEPLAY,
        MENU
    };

    inline constexpr unsigned int modeCount = 3;
    inline constexpr unsigned int gameplayBindingCount = 15;
    inline constexpr unsigned int menuBindingCount = 12;
    inline constexpr unsigned int bindingCount = gameplayBindingCount + menuBindingCount;
    inline constexpr unsigned int bindableButtons = 0xcfff;
    inline constexpr unsigned int cameraModifierIndex = 14;
    inline constexpr unsigned int encodedSize = 8 + modeCount * bindingCount * 4;

    struct TProfile {
        unsigned short bindings[bindingCount];
    };

    struct TConfig {
        EMode mode;
        TProfile profiles[modeCount];
    };

    struct TInputState {
        unsigned int suppressed;
        unsigned int chordConsumed;
        unsigned int previousButtons;
        EContext context;
        EMode mode;
        bool initialized;
    };

    struct TSample {
        short moveX;
        short moveY;
        short lookX;
        short lookY;
        unsigned int buttons;
    };

    static_assert(sizeof(TConfig) == 164, "Controls profile layout changed");
    static_assert(sizeof(TInputState) == 16, "Controls input state layout changed");
    static_assert(sizeof(TSample) == 12, "Native input sample layout changed");

    const char *ModeName(EMode mode);
    const char *ActionName(unsigned int index);
    void ResetProfile(TProfile &profile, EMode mode);
    void Initialize(TConfig &config);
    bool IsValidBinding(unsigned int buttons);
    bool SetBinding(TConfig &config, unsigned int index, unsigned int buttons);
    void Apply(const TConfig &config, TInputState &state, TSample &sample, EContext context);
    void Encode(const TConfig &config, char *text);
    bool Decode(TConfig &config, const char *text, unsigned int size);
    void DescribeBinding(unsigned int buttons, char *text);
}
