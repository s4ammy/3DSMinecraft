#include "ControlSettings.h"

namespace ControlSettings {
    struct TSettingSpec {
        const char *name;
        unsigned short minimum;
        unsigned short maximum;
        unsigned short step;
        unsigned short initial;
    };

    constexpr TSettingSpec settingSpecs[] = {
        {"Horizontal gain", 25, 200, 25, 100}, {"Vertical gain", 25, 200, 25, 100},
        {"Camera modifier", 0, 1, 1, 0}, {"Stick X", 72, 248, 16, 104}, {"Stick Y", 72, 168, 8, 160},
        {"Stick radius", 28, 68, 8, 52}, {"Dead zone", 0, 15, 1, 7}, {"Stick opacity", 0, 100, 5, 65},
        {"Hide idle stick", 0, 1, 1, 0}, {"Overlay", 0, 2, 1, 0}, {"Particles", 0, 2, 1, 0},
        {"Weather visuals", 0, 1, 1, 0}, {"Map refresh", 0, 3, 1, 0}, {"Low-memory textures", 0, 1, 1, 0},
        {"Mark shared binds", 0, 1, 1, 0}, {"Overlay corner", 0, 3, 1, 0},
        {"Overlay scale", 50, 125, 25, 100}, {"Background opacity", 0, 100, 5, 75},
        {"Camera response", 0, 2, 1, 0}, {"Outer-stick acceleration", 0, 100, 10, 0},
        {"Frame-time graph", 0, 1, 1, 0}, {"Render distance (chunks)", 1, 3, 1, 1},
        {"Nearby chunk radius", 1, 3, 1, 2}, {"Map radius (blocks)", 8, 32, 8, 24},
        {"Mesh builders", 1, 4, 1, 4}};
    constexpr unsigned short gameplayButtons[] = {1, 0x200, 0x100, 2, 0x400, 0x800, 0x20, 0x10, 0x40, 0x80, 8, 4, 0x4000, 0x8000, 0};
    constexpr unsigned short menuButtons[] = {1, 2, 0x400, 0x800, 0x200, 0x100, 0x40, 0x80, 0x20, 0x10, 8, 4, 0x4000, 0x8000};
    constexpr unsigned short physicalButtons[] = {1, 2, 0x400, 0x800, 0x200, 0x100, 0x40, 0x80, 0x20, 0x10, 8, 4, 0x4000, 0x8000};
    constexpr const char *physicalNames[] = {"A", "B", "X", "Y", "L", "R", "Up", "Down", "Left", "Right", "Start", "Select", "ZL", "ZR"};
    constexpr const char *actionNames[] = {
        "Jump / dismount", "Use / place", "Attack / mine", "Drop", "Inventory", "Crafting",
        "Previous item", "Next item", "Camera view", "Sneak / descend", "Pause", "Pause (alternate)",
        "Previous item (alt)", "Next item (alt)", "Camera modifier",
        "Confirm", "Back / cancel", "X action", "Y action", "Previous group", "Next group",
        "Navigate up", "Navigate down", "Navigate left", "Navigate right", "Start action", "Select action",
        "Previous hotbar slot", "Next hotbar slot", "Look up", "Look down", "Look left", "Look right"};

    char *Append(char *text, const char *value);
    int HexDigit(char value);
    char *WriteHex(char *text, unsigned int value);
    bool ReadHex(const char *&text, unsigned int &value);
}

const char *ControlSettings::ModeName(EMode mode) {
    switch (mode) {
    case EMode::L_CIRCLE_PAD:
        return "L + Circle Pad";
    case EMode::CIRCLE_PAD_PRO:
        return "Circle Pad Pro";
    case EMode::TOUCH_STICK:
        return "Touch stick";
    case EMode::ABXY_CAMERA:
        return "ABXY camera";
    default:
        return "Invalid";
    }
}

const char *ControlSettings::ActionName(unsigned int index) {
    return index < bindingCount ? actionNames[index] : "Invalid";
}

bool ControlSettings::IsGameplayBinding(unsigned int index) {
    return index < gameplayBindingCount || index >= cameraBindingBegin;
}

void ControlSettings::ResetProfile(TProfile &profile, EMode mode) {
    for (auto &binding : profile.bindings) {
        binding = 0;
    }

    for (auto index = 0U; index < gameplayBindingCount; ++index) {
        profile.bindings[index] = gameplayButtons[index];
    }

    for (auto index = 0U; index < menuBindingCount; ++index) {
        profile.bindings[gameplayBindingCount + index] = menuButtons[index];
    }

    if (mode == EMode::L_CIRCLE_PAD) {
        profile.bindings[1] = 2;
        profile.bindings[3] = 0xa00;
        profile.bindings[cameraModifierIndex] = 0x200;
        profile.bindings[gameplayBindingCount + 4] = 0;
    } else if (mode == EMode::ABXY_CAMERA) {
        profile.bindings[0] = 0x40;
        profile.bindings[3] = 0x280;
        profile.bindings[4] = 4;
        profile.bindings[5] = 0x300;
        profile.bindings[8] = 0;
        profile.bindings[11] = 0;
        profile.bindings[cameraBindingBegin] = 0x400;
        profile.bindings[cameraBindingBegin + 1] = 2;
        profile.bindings[cameraBindingBegin + 2] = 0x800;
        profile.bindings[cameraBindingBegin + 3] = 1;
    }
}

void ControlSettings::Initialize(TConfig &config) {
    for (auto index = 0U; index < modeCount; ++index) {
        config.mode = static_cast<EMode>(index);
        ResetMode(config);
    }

    config.mode = EMode::L_CIRCLE_PAD;
    for (auto index = 0U; index < globalSettingCount; ++index) {
        config.globalSettings[index] = settingSpecs[modeSettingCount + index].initial;
    }
}

void ControlSettings::ResetMode(TConfig &config) {
    const auto mode = static_cast<unsigned int>(config.mode);
    if (mode >= modeCount) {
        return;
    }

    ResetProfile(config.profiles[mode], config.mode);
    for (auto index = 0U; index < modeSettingCount; ++index) {
        config.modeSettings[mode][index] = settingSpecs[index].initial;
    }
}

bool ControlSettings::ResetBinding(TConfig &config, unsigned int index) {
    if (index >= bindingCount) {
        return false;
    }

    TProfile defaults;
    ResetProfile(defaults, config.mode);
    return SetBinding(config, index, defaults.bindings[index]);
}

bool ControlSettings::CopyBindings(TConfig &config, EMode source) {
    if (static_cast<unsigned int>(source) >= modeCount || static_cast<unsigned int>(config.mode) >= modeCount) {
        return false;
    }

    config.profiles[static_cast<unsigned int>(config.mode)] = config.profiles[static_cast<unsigned int>(source)];
    return true;
}

bool ControlSettings::IsShared(const TConfig &config, unsigned int index) {
    if (index >= bindingCount || static_cast<unsigned int>(config.mode) >= modeCount) {
        return false;
    }

    const auto &profile = config.profiles[static_cast<unsigned int>(config.mode)];
    const auto binding = profile.bindings[index];
    for (auto other = 0U; binding != 0 && other < bindingCount; ++other) {
        if (other != index && IsGameplayBinding(index) == IsGameplayBinding(other) && profile.bindings[other] == binding) {
            return true;
        }
    }

    return false;
}

const char *ControlSettings::SettingName(ESetting setting) {
    const auto index = static_cast<unsigned int>(setting);
    return index < static_cast<unsigned int>(ESetting::COUNT) ? settingSpecs[index].name : "Invalid";
}

unsigned int ControlSettings::GetSetting(const TConfig &config, ESetting setting) {
    const auto index = static_cast<unsigned int>(setting);
    if (index >= static_cast<unsigned int>(ESetting::COUNT) || static_cast<unsigned int>(config.mode) >= modeCount) {
        return 0;
    }

    return index < modeSettingCount ? config.modeSettings[static_cast<unsigned int>(config.mode)][index] :
                                      config.globalSettings[index - modeSettingCount];
}

bool ControlSettings::SetSetting(TConfig &config, ESetting setting, unsigned int value) {
    const auto index = static_cast<unsigned int>(setting);
    if (index >= static_cast<unsigned int>(ESetting::COUNT) || static_cast<unsigned int>(config.mode) >= modeCount ||
        value < settingSpecs[index].minimum || value > settingSpecs[index].maximum ||
        (setting == ESetting::MAP_RADIUS && value % 8 != 0)) {
        return false;
    }

    auto &destination = index < modeSettingCount ? config.modeSettings[static_cast<unsigned int>(config.mode)][index] :
                                                   config.globalSettings[index - modeSettingCount];
    destination = static_cast<unsigned short>(value);
    return true;
}

void ControlSettings::CycleSetting(TConfig &config, ESetting setting) {
    const auto index = static_cast<unsigned int>(setting);
    if (index >= static_cast<unsigned int>(ESetting::COUNT)) {
        return;
    }

    const auto next = GetSetting(config, setting) + settingSpecs[index].step;
    SetSetting(config, setting, next > settingSpecs[index].maximum ? settingSpecs[index].minimum : next);
}

void ControlSettings::ScaleLook(const TConfig &config, TSample &sample) {
    const auto curve = GetSetting(config, ESetting::CAMERA_CURVE);
    const auto acceleration = GetSetting(config, ESetting::CAMERA_ACCELERATION);
    const auto scale = [curve, acceleration](short value, unsigned int gain) {
        auto result = static_cast<int>(value) * static_cast<int>(gain) / 100;
        if (curve != 0 || acceleration != 0) {
            const auto magnitude = value < 0 ? -static_cast<int>(value) : static_cast<int>(value);
            const auto strength = magnitude > 100 ? 100 : magnitude;
            if (curve == 1) {
                result = result * strength / 100;
            } else if (curve == 2) {
                result = result * (200 - strength) / 100;
            }

            if (strength > 50) {
                result = result * (5000 + static_cast<int>(acceleration) * (strength - 50)) / 5000;
            }
        }

        result = result < -32768 ? -32768 : (result > 32767 ? 32767 : result);
        return static_cast<short>(result);
    };
    sample.lookX = scale(sample.lookX, GetSetting(config, ESetting::LOOK_X));
    sample.lookY = scale(sample.lookY, GetSetting(config, ESetting::LOOK_Y));
}

unsigned int ControlSettings::PerformancePreset(const TConfig &config) {
    constexpr unsigned short presets[][4] = {{0, 0, 0, 0}, {1, 1, 1, 0}, {2, 1, 2, 1}};
    for (auto preset = 0U; preset < 3; ++preset) {
        auto matches = true;
        for (auto index = 0U; index < 4; ++index) {
            matches &= GetSetting(config, static_cast<ESetting>(static_cast<unsigned int>(ESetting::PARTICLES) + index)) == presets[preset][index];
        }

        if (matches) {
            return preset + 1;
        }
    }

    return 0;
}

void ControlSettings::ApplyPerformancePreset(TConfig &config, unsigned int preset) {
    if (preset == 0 || preset > 3) {
        return;
    }

    constexpr unsigned short presets[][4] = {{0, 0, 0, 0}, {1, 1, 1, 0}, {2, 1, 2, 1}};
    for (auto index = 0U; index < 4; ++index) {
        SetSetting(config, static_cast<ESetting>(static_cast<unsigned int>(ESetting::PARTICLES) + index), presets[preset - 1][index]);
    }
}

bool ControlSettings::IsValidBinding(unsigned int buttons) {
    return (buttons & ~bindableButtons) == 0;
}

bool ControlSettings::SetBinding(TConfig &config, unsigned int index, unsigned int buttons) {
    if (index >= bindingCount || !IsValidBinding(buttons) || static_cast<unsigned int>(config.mode) >= modeCount) {
        return false;
    }

    auto &profile = config.profiles[static_cast<unsigned int>(config.mode)];
    profile.bindings[index] = static_cast<unsigned short>(buttons);
    return true;
}

void ControlSettings::Apply(const TConfig &config, TInputState &state, TSample &sample, EContext context) {
    if (config.mode != EMode::CIRCLE_PAD_PRO) {
        sample.lookX = 0;
        sample.lookY = 0;
    }

    const auto physical = sample.buttons & bindableButtons;
    const auto previousButtons = state.previousButtons;
    if (state.initialized && (state.context != context || state.mode != config.mode)) {
        //A held gameplay button must not become a fresh press when its menu opens.
        state.suppressed |= state.previousButtons;
        state.cameraLatched = false;
    }

    state.initialized = true;
    state.context = context;
    state.mode = config.mode;
    state.previousButtons = physical;
    state.suppressed &= physical;
    state.chordConsumed &= physical;
    const auto &profile = config.profiles[static_cast<unsigned int>(config.mode)];
    const auto begin = context == EContext::GAMEPLAY ? 0U : gameplayBindingCount;
    const auto faceCamera = config.mode == EMode::ABXY_CAMERA;
    const auto end = context == EContext::GAMEPLAY ? (faceCamera ? bindingCount : cameraModifierIndex) : cameraBindingBegin;
    auto chordButtons = 0U;
    const auto held = physical & ~state.suppressed;
    for (auto index = begin; index < end; ++index) {
        if (context == EContext::GAMEPLAY && index >= cameraModifierIndex && index < cameraBindingBegin) {
            continue;
        }

        const auto binding = profile.bindings[index];
        if (binding != 0 && (binding & (binding - 1)) != 0 && (held & binding) == binding) {
            chordButtons |= binding;
        }
    }

    auto result = sample.buttons & ~bindableButtons;
    auto directions = 0U;
    for (auto index = begin; index < end; ++index) {
        if (context == EContext::GAMEPLAY && index >= cameraModifierIndex && index < cameraBindingBegin) {
            continue;
        }

        const auto binding = profile.bindings[index];
        if (binding == 0 || (held & binding) != binding) {
            continue;
        }

        if ((binding & (binding - 1)) == 0 && (binding & (chordButtons | state.chordConsumed)) != 0) {
            continue;
        }

        if (index >= cameraBindingBegin) {
            directions |= 1U << (index - cameraBindingBegin);
        } else {
            result |= context == EContext::GAMEPLAY ? gameplayButtons[index] : menuButtons[index - begin];
        }
    }

    //Keep a chord's remaining buttons suppressed until each is released.
    state.chordConsumed |= chordButtons;
    if (context == EContext::GAMEPLAY) {
        const auto modifier = profile.bindings[cameraModifierIndex];
        const auto modifierHeld = modifier != 0 && (held & modifier) == modifier;
        const auto toggle = GetSetting(config, ESetting::MODIFIER_TOGGLE) != 0;
        if (toggle && modifierHeld && (previousButtons & modifier) != modifier) {
            state.cameraLatched = !state.cameraLatched;
        }

        if (config.mode == EMode::L_CIRCLE_PAD && (toggle ? state.cameraLatched : modifierHeld)) {
            sample.lookX = static_cast<short>(sample.moveX * 166 / 256);
            sample.lookY = static_cast<short>(sample.moveY * 166 / 256);
            sample.moveX = 0;
            sample.moveY = 0;
            result &= ~0xf0000000U;
        } else if (faceCamera) {
            const auto x = static_cast<int>((directions >> 3) & 1) - static_cast<int>((directions >> 2) & 1);
            const auto y = static_cast<int>(directions & 1) - static_cast<int>((directions >> 1) & 1);
            const auto speed = x != 0 && y != 0 ? 70 : 100;
            sample.lookX = static_cast<short>(x * speed);
            sample.lookY = static_cast<short>(y * speed);
        }
    }

    sample.buttons = result;
    ScaleLook(config, sample);
}

void ControlSettings::Encode(const TConfig &config, char *text) {
    const auto *digits = "0123456789abcdef";
    text = Append(text, "MC3DS4:");
    *text++ = digits[static_cast<unsigned int>(config.mode)];
    for (const auto &profile : config.profiles) {
        for (const auto binding : profile.bindings) {
            text = WriteHex(text, binding);
        }
    }

    for (const auto &settings : config.modeSettings) {
        for (const auto value : settings) {
            text = WriteHex(text, value);
        }
    }

    for (const auto value : config.globalSettings) {
        text = WriteHex(text, value);
    }

    *text = 0;
}

char *ControlSettings::WriteHex(char *text, unsigned int value) {
    const auto *digits = "0123456789abcdef";
    for (auto shift = 12; shift >= 0; shift -= 4) {
        *text++ = digits[(value >> shift) & 15];
    }

    return text;
}

bool ControlSettings::ReadHex(const char *&text, unsigned int &value) {
    value = 0;
    for (auto index = 0; index < 4; ++index) {
        const auto digit = HexDigit(*text++);
        if (digit < 0) {
            return false;
        }

        value = (value << 4) | static_cast<unsigned int>(digit);
    }

    return true;
}

int ControlSettings::HexDigit(char value) {
    if (value >= '0' && value <= '9') {
        return value - '0';
    }

    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }

    return -1;
}

bool ControlSettings::Decode(TConfig &config, const char *text, unsigned int size) {
    if (size != encodedSize && size != versionThreeEncodedSize && size != versionTwoEncodedSize && size != legacyEncodedSize) {
        return false;
    }

    const auto legacy = size == legacyEncodedSize;
    const auto versionTwo = size == versionTwoEncodedSize;
    const auto versionThree = size == versionThreeEncodedSize;
    const auto *prefix = legacy ? "MC3DS1:" : versionTwo ? "MC3DS2:" : versionThree ? "MC3DS3:" : "MC3DS4:";
    for (auto index = 0U; index < 7; ++index) {
        if (text[index] != prefix[index]) {
            return false;
        }
    }

    if (text[7] < '0' || text[7] >= '0' + (legacy ? legacyModeCount : modeCount)) {
        return false;
    }

    TConfig candidate;
    Initialize(candidate);
    candidate.mode = static_cast<EMode>(text[7] - '0');
    text += 8;
    for (auto mode = 0U; mode < (legacy ? legacyModeCount : modeCount); ++mode) {
        auto &profile = candidate.profiles[mode];
        for (auto index = 0U; index < (legacy ? legacyBindingCount : bindingCount); ++index) {
            auto value = 0U;
            if (!ReadHex(text, value) || !IsValidBinding(value)) {
                return false;
            }

            profile.bindings[index] = static_cast<unsigned short>(value);
        }
    }

    if (!legacy) {
        const auto savedMode = candidate.mode;
        for (auto mode = 0U; mode < modeCount; ++mode) {
            candidate.mode = static_cast<EMode>(mode);
            for (auto index = 0U; index < modeSettingCount; ++index) {
                unsigned int value;
                if (!ReadHex(text, value) || !SetSetting(candidate, static_cast<ESetting>(index), value)) {
                    return false;
                }
            }
        }

        const auto end = static_cast<unsigned int>(versionTwo ? ESetting::OVERLAY_POSITION : versionThree ? ESetting::RENDER_DISTANCE : ESetting::COUNT);
        for (auto index = modeSettingCount; index < end; ++index) {
            unsigned int value;
            if (!ReadHex(text, value) || !SetSetting(candidate, static_cast<ESetting>(index), value)) {
                return false;
            }
        }

        candidate.mode = savedMode;
    }

    config = candidate;
    return true;
}

char *ControlSettings::Append(char *text, const char *value) {
    while (*value != 0) {
        *text++ = *value++;
    }

    *text = 0;
    return text;
}

void ControlSettings::DescribeBinding(unsigned int buttons, char *text) {
    if (buttons == 0) {
        Append(text, "Unbound");
        return;
    }

    auto first = true;
    for (auto index = 0U; index < sizeof(physicalButtons) / sizeof(physicalButtons[0]); ++index) {
        if ((buttons & physicalButtons[index]) == 0) {
            continue;
        }

        if (!first) {
            text = Append(text, "+");
        }

        text = Append(text, physicalNames[index]);
        first = false;
    }
}
