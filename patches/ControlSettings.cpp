#include "ControlSettings.h"

namespace ControlSettings {
    constexpr unsigned short gameplayButtons[] = {1, 0x200, 0x100, 2, 0x400, 0x800, 0x20, 0x10, 0x40, 0x80, 8, 4, 0x4000, 0x8000, 0};
    constexpr unsigned short menuButtons[] = {1, 2, 0x400, 0x800, 0x200, 0x100, 0x40, 0x80, 0x20, 0x10, 8, 4};
    constexpr unsigned short physicalButtons[] = {1, 2, 0x400, 0x800, 0x200, 0x100, 0x40, 0x80, 0x20, 0x10, 8, 4, 0x4000, 0x8000};
    constexpr const char *physicalNames[] = {"A", "B", "X", "Y", "L", "R", "Up", "Down", "Left", "Right", "Start", "Select", "ZL", "ZR"};
    constexpr const char *actionNames[] = {
        "Jump / dismount", "Use / place", "Attack / mine", "Drop", "Inventory", "Crafting",
        "Previous item", "Next item", "Camera view", "Sneak / descend", "Pause", "Pause (alternate)",
        "Previous item (alt)", "Next item (alt)", "Camera modifier",
        "Confirm", "Back / cancel", "X action", "Y action", "Previous group", "Next group",
        "Navigate up", "Navigate down", "Navigate left", "Navigate right", "Start action", "Select action"
    };

    char *Append(char *text, const char *value);
    int HexDigit(char value);
}

const char *ControlSettings::ModeName(EMode mode) {
    switch (mode) {
    case EMode::L_CIRCLE_PAD:
        return "L + Circle Pad";
    case EMode::CIRCLE_PAD_PRO:
        return "Circle Pad Pro";
    case EMode::TOUCH_STICK:
        return "Touch stick";
    default:
        return "Invalid";
    }
}

const char *ControlSettings::ActionName(unsigned int index) {
    return index < bindingCount ? actionNames[index] : "Invalid";
}

void ControlSettings::ResetProfile(TProfile &profile, EMode mode) {
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
    }
}

void ControlSettings::Initialize(TConfig &config) {
    config.mode = EMode::L_CIRCLE_PAD;
    for (auto index = 0U; index < modeCount; ++index) {
        ResetProfile(config.profiles[index], static_cast<EMode>(index));
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
    if (state.initialized && (state.context != context || state.mode != config.mode)) {
        //A held gameplay button must not become a fresh press when its menu opens.
        state.suppressed |= state.previousButtons;
    }

    state.initialized = true;
    state.context = context;
    state.mode = config.mode;
    state.previousButtons = physical;
    state.suppressed &= physical;
    state.chordConsumed &= physical;
    const auto &profile = config.profiles[static_cast<unsigned int>(config.mode)];
    const auto begin = context == EContext::GAMEPLAY ? 0U : gameplayBindingCount;
    const auto end = context == EContext::GAMEPLAY ? cameraModifierIndex : bindingCount;
    auto chordButtons = 0U;
    const auto held = physical & ~state.suppressed;
    for (auto index = begin; index < end; ++index) {
        const auto binding = profile.bindings[index];
        if (binding != 0 && (binding & (binding - 1)) != 0 && (held & binding) == binding) {
            chordButtons |= binding;
        }
    }

    auto result = sample.buttons & ~bindableButtons;
    for (auto index = begin; index < end; ++index) {
        const auto binding = profile.bindings[index];
        if (binding == 0 || (held & binding) != binding) {
            continue;
        }

        if ((binding & (binding - 1)) == 0 && (binding & (chordButtons | state.chordConsumed)) != 0) {
            continue;
        }

        result |= context == EContext::GAMEPLAY ? gameplayButtons[index] : menuButtons[index - begin];
    }

    //Keep a chord's remaining buttons suppressed until each is released.
    state.chordConsumed |= chordButtons;
    if (context == EContext::GAMEPLAY) {
        const auto modifier = profile.bindings[cameraModifierIndex];
        if (config.mode == EMode::L_CIRCLE_PAD && modifier != 0 && (held & modifier) == modifier) {
            sample.lookX = static_cast<short>(sample.moveX * 166 / 256);
            sample.lookY = static_cast<short>(sample.moveY * 166 / 256);
            sample.moveX = 0;
            sample.moveY = 0;
            result &= ~0xf0000000U;
        }
    }

    sample.buttons = result;
}

void ControlSettings::Encode(const TConfig &config, char *text) {
    const auto *digits = "0123456789abcdef";
    text = Append(text, "MC3DS1:");
    *text++ = digits[static_cast<unsigned int>(config.mode)];
    for (const auto &profile : config.profiles) {
        for (const auto binding : profile.bindings) {
            for (auto shift = 12; shift >= 0; shift -= 4) {
                *text++ = digits[(binding >> shift) & 15];
            }
        }
    }

    *text = 0;
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
    if (size != encodedSize) {
        return false;
    }

    const auto *prefix = "MC3DS1:";
    for (auto index = 0U; index < 7; ++index) {
        if (text[index] != prefix[index]) {
            return false;
        }
    }

    if (text[7] < '0' || text[7] >= '0' + modeCount) {
        return false;
    }

    TConfig candidate;
    candidate.mode = static_cast<EMode>(text[7] - '0');
    text += 8;
    for (auto &profile : candidate.profiles) {
        for (auto &binding : profile.bindings) {
            auto value = 0U;
            for (auto index = 0; index < 4; ++index) {
                const auto digit = HexDigit(*text++);
                if (digit < 0) {
                    return false;
                }

                value = (value << 4) | static_cast<unsigned int>(digit);
            }

            if (!IsValidBinding(value)) {
                return false;
            }

            binding = static_cast<unsigned short>(value);
        }
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
