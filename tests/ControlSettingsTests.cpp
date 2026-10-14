#include "controls/ControlSettings.h"

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace ControlSettingsTests {
    void Require(bool condition, const char *message);
    void CheckDefaults();
    void CheckBindings();
    void CheckSharedBindings();
    void CheckEveryAction();
    void CheckPersistence();
}

void ControlSettingsTests::Require(bool condition, const char *message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void ControlSettingsTests::CheckDefaults() {
    ControlSettings::TConfig config;
    ControlSettings::Initialize(config);
    Require(config.mode == ControlSettings::EMode::L_CIRCLE_PAD, "Wrong first-use mode");
    const auto apply = [&](unsigned int buttons) {
        ControlSettings::TInputState input{};
        ControlSettings::TSample sample{100, -100, 80, -80, buttons};
        ControlSettings::Apply(config, input, sample, ControlSettings::EContext::GAMEPLAY);
        return sample;
    };
    Require(apply(2).buttons == 0x200, "B must use/place with the L+Circle Pad defaults");
    Require(apply(0xa00).buttons == 2, "L+Y must drop without crafting");
    Require(apply(0x800).buttons == 0x800, "Y must craft");
    Require(apply(0x100).buttons == 0x100, "R must attack");
    Require(apply(0).lookX == 0, "IR input leaked into non-accessory mode");
    const auto aiming = apply(0x200);
    Require(aiming.moveX == 0 && aiming.moveY == 0 && aiming.lookX == 64, "Camera modifier did not redirect the Circle Pad");

    for (const auto mode : {ControlSettings::EMode::CIRCLE_PAD_PRO, ControlSettings::EMode::TOUCH_STICK}) {
        config.mode = mode;
        for (const auto buttons : {1U, 2U, 0x100U, 0x200U, 0x400U, 0x800U, 0x4000U, 0x8000U}) {
            Require(apply(buttons).buttons == buttons, "Stock physical binding changed");
        }
    }
}

void ControlSettingsTests::CheckBindings() {
    ControlSettings::TConfig config;
    ControlSettings::Initialize(config);
    Require(ControlSettings::SetBinding(config, 0, 0), "Cannot unbind an action");
    Require(ControlSettings::SetBinding(config, 0, 0x401), "Cannot bind a chord");
    Require(!ControlSettings::SetBinding(config, 0, 0x10000), "Invalid hardware bit accepted");
    Require(!ControlSettings::SetBinding(config, ControlSettings::bindingCount, 1), "Invalid action accepted");
    Require(ControlSettings::SetBinding(config, 12, 0), "Cannot unbind ZL alternate");
    ControlSettings::TInputState input{};
    const auto apply = [&](unsigned int buttons, ControlSettings::EContext context = ControlSettings::EContext::GAMEPLAY) {
        ControlSettings::TSample sample{0, 0, 0, 0, buttons};
        ControlSettings::Apply(config, input, sample, context);
        return sample.buttons;
    };
    Require(apply(0x401) == 1, "Chord did not take precedence over its component binding");
    Require(apply(0x400) == 0, "Partial chord release opened inventory");
    Require(apply(0) == 0 && apply(0x400) == 0x400, "Chord suppression did not release");
    Require(apply(0x400, ControlSettings::EContext::MENU) == 0, "Held gameplay input became a menu press");
    Require(apply(0, ControlSettings::EContext::MENU) == 0 && apply(0x400, ControlSettings::EContext::MENU) == 0x400,
        "Menu input did not recover after release");
    Require(apply(0x4000) == 0, "Unbound ZL still activated hotbar");
    config.mode = ControlSettings::EMode::TOUCH_STICK;
    Require(config.profiles[2].bindings[0] == 1, "Editing one profile changed another");
    ControlSettings::ResetProfile(config.profiles[0], ControlSettings::EMode::L_CIRCLE_PAD);
    Require(config.profiles[0].bindings[0] == 1, "Reset did not restore defaults");
    std::array<char, 160> label{};
    ControlSettings::DescribeBinding(0xa00, label.data());
    Require(std::strcmp(label.data(), "Y+L") == 0, "Chord label is wrong");
}

void ControlSettingsTests::CheckSharedBindings() {
    for (auto mode = 0U; mode < ControlSettings::modeCount; ++mode) {
        ControlSettings::TConfig config;
        ControlSettings::Initialize(config);
        config.mode = static_cast<ControlSettings::EMode>(mode);
        const auto defaults = config;
        Require(ControlSettings::SetBinding(config, 0, 2), "Cannot bind Jump to the already-bound B button");
        for (auto otherMode = 0U; otherMode < ControlSettings::modeCount; ++otherMode) {
            for (auto index = 0U; index < ControlSettings::bindingCount; ++index) {
                const auto expected = otherMode == mode && index == 0 ? 2 : defaults.profiles[otherMode].bindings[index];
                Require(config.profiles[otherMode].bindings[index] == expected, "Sharing B changed another binding");
            }
        }

        const auto apply = [&](unsigned int buttons, ControlSettings::EContext context = ControlSettings::EContext::GAMEPLAY) {
            ControlSettings::TInputState input{};
            ControlSettings::TSample sample{0, 0, 0, 0, buttons};
            ControlSettings::Apply(config, input, sample, context);
            return sample.buttons;
        };
        const auto originalAction = mode == 0 ? 0x200U : (mode == 3 ? 0U : 2U);
        Require(apply(2) == (1 | originalAction), "Shared B did not activate both gameplay actions");
        Require(apply(1) == 0, "Jump's old binding remained active");
        Require(ControlSettings::SetBinding(config, 2, 2), "Cannot assign three actions to B");
        Require(apply(2) == (0x101 | originalAction), "Shared B did not activate all three actions");
        Require(ControlSettings::SetBinding(config, 0, 0), "Cannot unbind a shared action");
        Require(apply(2) == (0x100 | originalAction), "Unbinding one action changed another shared action");
        Require(ControlSettings::SetBinding(config, 0, 0x401), "Cannot bind a gameplay chord");
        Require(ControlSettings::SetBinding(config, 2, 0x401), "Cannot share a gameplay chord");
        Require(apply(0x401) == 0x101, "Shared chords did not activate both actions and suppress their components");

        const auto confirmIndex = ControlSettings::gameplayBindingCount;
        Require(ControlSettings::SetBinding(config, confirmIndex, 2), "Cannot share a menu button");
        Require(config.profiles[mode].bindings[confirmIndex + 1] == 2, "Sharing a menu button changed Cancel");
        Require(apply(2, ControlSettings::EContext::MENU) == 3, "Shared menu button did not emit both actions");
        Require(apply(2) == originalAction, "Menu sharing changed gameplay bindings");
        Require(ControlSettings::SetBinding(config, confirmIndex, 0x401), "Cannot bind a menu chord");
        Require(ControlSettings::SetBinding(config, confirmIndex + 1, 0x401), "Cannot share a menu chord");
        Require(apply(0x401, ControlSettings::EContext::MENU) == 3, "Shared menu chord did not emit both actions");
    }
}

void ControlSettingsTests::CheckPersistence() {
    ControlSettings::TConfig original;
    ControlSettings::Initialize(original);
    original.mode = ControlSettings::EMode::TOUCH_STICK;
    Require(ControlSettings::SetBinding(original, 0, 0x201), "Custom binding rejected");
    Require(ControlSettings::SetBinding(original, 2, 0x201), "Shared chord rejected before save");
    Require(ControlSettings::SetBinding(original, ControlSettings::gameplayBindingCount, 2), "Shared button rejected before save");
    std::array<char, ControlSettings::encodedSize + 1> encoded{};
    ControlSettings::Encode(original, encoded.data());
    Require(std::strlen(encoded.data()) == ControlSettings::encodedSize, "Encoded size differs");
    ControlSettings::TConfig restored;
    ControlSettings::Initialize(restored);
    Require(ControlSettings::Decode(restored, encoded.data(), ControlSettings::encodedSize), "Cannot load valid settings");
    std::array<char, ControlSettings::encodedSize + 1> roundTrip{};
    ControlSettings::Encode(restored, roundTrip.data());
    Require(encoded == roundTrip, "Settings did not round-trip");
    for (const auto index : {0U, 7U, 8U, ControlSettings::encodedSize - 1}) {
        auto malformed = encoded;
        malformed[index] = '!';
        Require(!ControlSettings::Decode(restored, malformed.data(), ControlSettings::encodedSize), "Malformed settings accepted");
        ControlSettings::Encode(restored, roundTrip.data());
        Require(encoded == roundTrip, "Malformed settings partially changed the live profile");
    }
    Require(!ControlSettings::Decode(restored, encoded.data(), 0), "Empty settings accepted");
    Require(!ControlSettings::Decode(restored, encoded.data(), ControlSettings::encodedSize - 1), "Truncated settings accepted");
}

void ControlSettingsTests::CheckEveryAction() {
    constexpr unsigned int nativeButtons[] = {
        1, 0x200, 0x100, 2, 0x400, 0x800, 0x20, 0x10, 0x40, 0x80, 8, 4, 0x4000, 0x8000, 0,
        1, 2, 0x400, 0x800, 0x200, 0x100, 0x40, 0x80, 0x20, 0x10, 8, 4, 0x4000, 0x8000, 0, 0, 0, 0};
    for (auto mode = 0U; mode < ControlSettings::modeCount; ++mode) {
        for (auto index = 0U; index < ControlSettings::bindingCount; ++index) {
            ControlSettings::TConfig config;
            ControlSettings::Initialize(config);
            config.mode = static_cast<ControlSettings::EMode>(mode);
            for (auto &binding : config.profiles[mode].bindings) {
                binding = 0;
            }

            Require(ControlSettings::SetBinding(config, index, 1), "An exposed action cannot be rebound");
            ControlSettings::TInputState input{};
            ControlSettings::TSample sample{100, -100, 70, -70, 1};
            const auto context = ControlSettings::IsGameplayBinding(index) ?
                ControlSettings::EContext::GAMEPLAY :
                ControlSettings::EContext::MENU;
            ControlSettings::Apply(config, input, sample, context);
            Require(sample.buttons == nativeButtons[index], "An exposed action produced the wrong native button");
            if (index == ControlSettings::cameraModifierIndex && mode == 0) {
                Require(sample.moveX == 0 && sample.lookX == 64 && sample.lookY == -64,
                    "Custom camera modifier did not redirect both axes");
            }

            config.mode = static_cast<ControlSettings::EMode>((mode + 1) % ControlSettings::modeCount);
            sample.buttons = 1;
            ControlSettings::Apply(config, input, sample, context);
            Require(sample.buttons == 0, "Changing modes reinterpreted a held button");
        }
    }
}

int main() {
    try {
        ControlSettingsTests::CheckDefaults();
        ControlSettingsTests::CheckBindings();
        ControlSettingsTests::CheckSharedBindings();
        ControlSettingsTests::CheckEveryAction();
        ControlSettingsTests::CheckPersistence();
        std::cout << "Control defaults, remapping, shared bindings, chords, recovery and persistence passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
