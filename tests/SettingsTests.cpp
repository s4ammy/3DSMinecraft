#include "ControlSettings.h"
#include "RuntimePrompts.h"
#include "RuntimeVisuals.h"
#include "TouchStick.h"

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace SettingsTests {
    using ESetting = ControlSettings::ESetting;
    void Require(bool condition, const char *message);
    void CheckSettings();
    void CheckToggle();
    void CheckMigration();
    void CheckTouch();
    void CheckTextures();
    void CheckPrompts();
    void CheckAbxy();
    void CheckCameraAndPresets();
}

void SettingsTests::Require(bool condition, const char *message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void SettingsTests::CheckSettings() {
    ControlSettings::TConfig config;
    ControlSettings::Initialize(config);
    const auto initial = config;
    for (auto mode = 0U; mode < ControlSettings::modeCount; ++mode) {
        config.mode = static_cast<ControlSettings::EMode>(mode);
        Require(ControlSettings::GetSetting(config, ESetting::LOOK_X) == 100, "Default gain changed");
        Require(ControlSettings::GetSetting(config, ESetting::TOUCH_RADIUS) == 52, "Default radius changed");
        for (auto index = 0U; index < static_cast<unsigned int>(ESetting::COUNT); ++index) {
            const auto setting = static_cast<ESetting>(index);
            Require(!ControlSettings::SetSetting(config, setting, 0xffffffff), "Setting overflow accepted");
            auto returned = false;
            const auto old = ControlSettings::GetSetting(config, setting);
            for (auto step = 0; step < 150; ++step) {
                ControlSettings::CycleSetting(config, setting);
                if (ControlSettings::GetSetting(config, setting) == old) {
                    returned = true;
                    break;
                }
            }

            Require(returned, "Setting cycle cannot reach original value");
        }
    }

    config.mode = initial.mode;
    Require(!ControlSettings::SetSetting(config, ESetting::LOOK_X, 24), "Out-of-range gain accepted");
    Require(!ControlSettings::SetSetting(config, ESetting::COUNT, 0), "Invalid setting accepted");
    Require(!ControlSettings::SetSetting(config, ESetting::MAP_RADIUS, 9), "Unsupported map step accepted");
    for (const auto setting : {ESetting::RENDER_DISTANCE, ESetting::CHUNK_RADIUS, ESetting::MESH_BUILDERS}) {
        Require(!ControlSettings::SetSetting(config, setting, 0), "Zero world limit accepted");
    }
    Require(ControlSettings::SetSetting(config, ESetting::LOOK_X, 200), "Valid gain rejected");
    Require(ControlSettings::SetSetting(config, ESetting::LOOK_Y, 25), "Valid vertical gain rejected");
    ControlSettings::TSample sample{0, 0, 20000, -100, 0};
    ControlSettings::ScaleLook(config, sample);
    Require(sample.lookX == 32767 && sample.lookY == -25, "Independent gains/saturation differ");
    sample.lookX = -32768;
    ControlSettings::ScaleLook(config, sample);
    Require(sample.lookX == -32768, "Negative gain saturation differs");
    ControlSettings::SetSetting(config, ESetting::OVERLAY, 2);
    ControlSettings::SetBinding(config, 0, 2);
    Require(ControlSettings::IsShared(config, 0), "Shared binding not detected");
    Require(ControlSettings::ResetBinding(config, 0) && !ControlSettings::IsShared(config, 0), "Single binding reset differs");
    Require(ControlSettings::CopyBindings(config, ControlSettings::EMode::CIRCLE_PAD_PRO), "Copy failed");
    Require(config.profiles[0].bindings[1] == 0x200, "Copy did not copy bindings");
    Require(ControlSettings::GetSetting(config, ESetting::LOOK_X) == 200, "Copy changed camera tuning");
    ControlSettings::ResetMode(config);
    Require(ControlSettings::GetSetting(config, ESetting::LOOK_X) == 100, "Mode reset did not reset tuning");
    Require(ControlSettings::GetSetting(config, ESetting::OVERLAY) == 2, "Mode reset changed global setting");
    Require(!ControlSettings::CopyBindings(config, ControlSettings::EMode::COUNT), "Invalid copy source accepted");
}

void SettingsTests::CheckToggle() {
    ControlSettings::TConfig config;
    ControlSettings::Initialize(config);
    ControlSettings::SetSetting(config, ESetting::MODIFIER_TOGGLE, 1);
    ControlSettings::TInputState state{};
    const auto apply = [&](unsigned int keys, ControlSettings::EContext context = ControlSettings::EContext::GAMEPLAY) {
        ControlSettings::TSample sample{100, -100, 0, 0, keys};
        ControlSettings::Apply(config, state, sample, context);
        return sample;
    };
    Require(apply(0x200).lookX == 64 && state.cameraLatched, "Toggle press did not latch");
    Require(apply(0x200).lookX == 64 && state.cameraLatched, "Held toggle repeated");
    Require(apply(0).lookX == 64 && state.cameraLatched, "Toggle release cleared latch");
    Require(apply(0x200).lookX == 0 && !state.cameraLatched, "Second press did not unlatch");
    apply(0);
    apply(0x200);
    apply(0, ControlSettings::EContext::MENU);
    Require(!state.cameraLatched, "Menu transition did not clear latch");
    Require(apply(0).lookX == 0, "Closing menu resumed latched camera");
    ControlSettings::SetBinding(config, ControlSettings::cameraModifierIndex, 0x201);
    apply(0x200);
    Require(!state.cameraLatched && apply(0x201).lookX == 64, "Chord modifier rising edge differs");
    apply(0x201);
    Require(state.cameraLatched, "Held modifier chord toggled twice");
    config.mode = ControlSettings::EMode::TOUCH_STICK;
    apply(0x201);
    Require(!state.cameraLatched, "Mode transition kept camera latch");
}

void SettingsTests::CheckMigration() {
    ControlSettings::TConfig config;
    ControlSettings::Initialize(config);
    config.mode = ControlSettings::EMode::TOUCH_STICK;
    ControlSettings::SetBinding(config, 0, 2);
    ControlSettings::SetSetting(config, ESetting::TOUCH_X, 248);
    ControlSettings::SetSetting(config, ESetting::PARTICLES, 2);
    std::array<char, ControlSettings::encodedSize + 1> saved{};
    ControlSettings::Encode(config, saved.data());
    std::array<char, ControlSettings::legacyEncodedSize + 1> legacy{};
    std::memcpy(legacy.data(), "MC3DS1:2", 8);
    for (auto mode = 0U; mode < 3; ++mode) {
        std::memcpy(legacy.data() + 8 + mode * 108, saved.data() + 8 + mode * ControlSettings::bindingCount * 4, 108);
    }

    ControlSettings::TConfig restored;
    Require(ControlSettings::Decode(restored, legacy.data(), legacy.size() - 1), "V1 migration failed");
    Require(restored.mode == config.mode && restored.profiles[2].bindings[0] == 2, "Migration lost mode/binding");
    Require(restored.profiles[2].bindings[27] == 0x4000 && restored.profiles[2].bindings[28] == 0x8000, "New menu actions lack defaults");
    Require(ControlSettings::GetSetting(restored, ESetting::TOUCH_X) == 104, "Migration did not default new settings");
    std::array<char, ControlSettings::versionTwoEncodedSize + 1> versionTwo{};
    std::memcpy(versionTwo.data(), saved.data(), versionTwo.size() - 1);
    versionTwo[5] = '2';
    Require(ControlSettings::Decode(restored, versionTwo.data(), versionTwo.size() - 1), "V2 migration failed");
    Require(ControlSettings::GetSetting(restored, ESetting::OVERLAY_SCALE) == 100, "V2 migration lost new defaults");
    Require(restored.profiles[2].bindings[0] == 2, "V2 migration lost shared binding");
    Require(ControlSettings::GetSetting(restored, ESetting::TOUCH_X) == 248, "V2 migration lost mode tuning");
    std::array<char, ControlSettings::versionThreeEncodedSize + 1> versionThree{};
    std::memcpy(versionThree.data(), saved.data(), versionThree.size() - 1);
    versionThree[5] = '3';
    Require(ControlSettings::Decode(restored, versionThree.data(), versionThree.size() - 1), "V3 migration failed");
    Require(ControlSettings::GetSetting(restored, ESetting::RENDER_DISTANCE) == 1 &&
        ControlSettings::GetSetting(restored, ESetting::CHUNK_RADIUS) == 2 &&
        ControlSettings::GetSetting(restored, ESetting::MAP_RADIUS) == 24 &&
        ControlSettings::GetSetting(restored, ESetting::MESH_BUILDERS) == 4, "V3 migration changed world defaults");
    Require(ControlSettings::GetSetting(restored, ESetting::TOUCH_X) == 248 &&
        restored.profiles[2].bindings[0] == 2, "V3 migration lost existing settings");
    Require(ControlSettings::Decode(restored, saved.data(), saved.size() - 1), "V4 round trip failed");
    const auto before = saved;
    const auto settingsOffset = 8 + ControlSettings::modeCount * ControlSettings::bindingCount * 4;
    for (auto index = settingsOffset; index < ControlSettings::encodedSize; index += 4) {
        auto bad = saved;
        std::memcpy(bad.data() + index, "ffff", 4);
        Require(!ControlSettings::Decode(restored, bad.data(), bad.size() - 1), "Out-of-range saved setting accepted");
        ControlSettings::Encode(restored, bad.data());
        Require(bad == before, "Rejected save changed live state");
    }
}

void SettingsTests::CheckCameraAndPresets() {
    ControlSettings::TConfig config;
    ControlSettings::Initialize(config);
    const auto before = config.profiles[0];
    ControlSettings::SetSetting(config, ESetting::CHUNK_RADIUS, 3);
    ControlSettings::SetSetting(config, ESetting::RENDER_DISTANCE, 2);
    ControlSettings::SetSetting(config, ESetting::MAP_RADIUS, 8);
    ControlSettings::SetSetting(config, ESetting::MESH_BUILDERS, 1);
    for (auto preset = 1U; preset <= 3; ++preset) {
        ControlSettings::ApplyPerformancePreset(config, preset);
        Require(ControlSettings::PerformancePreset(config) == preset, "Preset detection differs");
        Require(std::memcmp(&before, &config.profiles[0], sizeof(before)) == 0, "Preset changed bindings");
        Require(ControlSettings::GetSetting(config, ESetting::CHUNK_RADIUS) == 3 &&
            ControlSettings::GetSetting(config, ESetting::RENDER_DISTANCE) == 2 &&
            ControlSettings::GetSetting(config, ESetting::MAP_RADIUS) == 8 &&
            ControlSettings::GetSetting(config, ESetting::MESH_BUILDERS) == 1, "Visual preset changed world settings");
    }

    ControlSettings::SetSetting(config, ESetting::MAP_RATE, 3);
    Require(ControlSettings::PerformancePreset(config) == 0, "Custom preset not detected");
    for (auto curve = 0U; curve < 3; ++curve) {
        ControlSettings::SetSetting(config, ESetting::CAMERA_CURVE, curve);
        for (auto acceleration : {0U, 50U, 100U}) {
            ControlSettings::SetSetting(config, ESetting::CAMERA_ACCELERATION, acceleration);
            auto previous = 0;
            for (auto magnitude = 0; magnitude <= 100; ++magnitude) {
                ControlSettings::TSample sample{0, 0, static_cast<short>(magnitude), static_cast<short>(-magnitude), 0};
                ControlSettings::ScaleLook(config, sample);
                Require(sample.lookX == -sample.lookY, "Response curve is asymmetric");
                Require(sample.lookX >= previous, "Response curve is non-monotonic");
                previous = sample.lookX;
            }
        }
    }
}

void SettingsTests::CheckTouch() {
    for (const auto settings : {TouchStick::TSettings{72, 72, 28, 0}, TouchStick::TSettings{248, 168, 68, 15}}) {
        for (auto y = 0; y < 240; ++y) {
            for (auto x = 0; x < 320; ++x) {
                TouchStick::TInputState input{};
                const auto dx = x - settings.x;
                const auto dy = y - settings.y;
                const auto inside = dx * dx + dy * dy <= settings.radius * settings.radius;
                Require(TouchStick::UpdateConfigured(input, x, y, true, true, settings) == inside, "Custom capture region differs");
                Require(input.lookX * input.lookX + input.lookY * input.lookY <= 10000, "Custom diagonal speed exceeds maximum");
            }
        }
    }

    TouchStick::TInputState input{};
    Require(!TouchStick::UpdateConfigured(input, 104, 160, true, true, {104, 160, 8, 7}), "Invalid dead zone captured gesture");
}

void SettingsTests::CheckTextures() {
    for (const auto format : {0U, 4U}) {
        const auto baseBytes = 256U * 256U * (format == 0 ? 4U : 2U);
        std::array<unsigned int, 8> header{0x54534433, 3, format, 256, 256, 256, 256, 3};
        const auto fileBytes = 32 + baseBytes + baseBytes / 4 + baseBytes / 16;
        RuntimeVisuals::TTexturePlan plan{};
        Require(RuntimeVisuals::PlanTerrainTexture(header.data(), fileBytes, "romfs:/atlas/atlas.terrain.meta_BE1B9B01_2.3dst", plan), "Valid terrain texture rejected");
        Require(plan.skippedBytes == baseBytes && plan.payloadBytes == baseBytes / 4 + baseBytes / 16, "Mip payload calculation differs");
        for (const auto name : {"", "atlas.terrain.meta_", "atlas.terrain.meta_x.3dst", "atlas.terrain.meta_1.3dst.extra", "gui.3dst"}) {
            Require(!RuntimeVisuals::PlanTerrainTexture(header.data(), fileBytes, name, plan), "Non-terrain texture accepted");
        }

        Require(!RuntimeVisuals::PlanTerrainTexture(header.data(), fileBytes - 1, "atlas.terrain.meta_BE1B9B01_2.3dst", plan), "Truncated terrain mip accepted");
        for (auto index = 0U; index < header.size(); ++index) {
            auto bad = header;
            bad[index] = 0xffffffff;
            Require(!RuntimeVisuals::PlanTerrainTexture(bad.data(), fileBytes, "atlas.terrain.meta_BE1B9B01_2.3dst", plan), "Invalid terrain header accepted");
        }
    }
}

void SettingsTests::CheckPrompts() {
    ControlSettings::TConfig config;
    ControlSettings::Initialize(config);
    Require(RuntimePrompts::BindingForGlyph(config, 8, true) == 2, "Use prompt did not show B");
    Require(RuntimePrompts::BindingForGlyph(config, 1, true) == 0xa00, "Drop prompt lost modifier chord");
    Require(RuntimePrompts::BindingForGlyph(config, 8, false) == 0, "Unbound menu action still has a prompt");
    Require(RuntimePrompts::BindingForGlyph(config, 10, false) == 0x4000, "Menu hotbar prompt differs");
    ControlSettings::SetBinding(config, 15, 0x401);
    Require(RuntimePrompts::BindingForGlyph(config, 0, false) == 0x401, "Custom menu chord not reflected");
    Require(RuntimePrompts::GlyphForButton(2) == 1 && RuntimePrompts::GlyphForButton(0x401) == -1 &&
            RuntimePrompts::GlyphForButton(0) == -1,
        "Prompt selected a misleading single-key glyph");
}

void SettingsTests::CheckAbxy() {
    ControlSettings::TConfig config;
    ControlSettings::Initialize(config);
    config.mode = ControlSettings::EMode::ABXY_CAMERA;
    const auto apply = [&](unsigned int buttons, ControlSettings::EContext context = ControlSettings::EContext::GAMEPLAY) {
        ControlSettings::TInputState state{};
        ControlSettings::TSample sample{67, -54, 80, 80, buttons};
        ControlSettings::Apply(config, state, sample, context);
        Require(sample.moveX == 67 && sample.moveY == -54, "ABXY camera stopped movement");
        return sample;
    };
    Require(apply(0x400).lookY == 100 && apply(2).lookY == -100, "ABXY vertical defaults differ");
    Require(apply(0x800).lookX == -100 && apply(1).lookX == 100, "ABXY horizontal defaults differ");
    Require(apply(0xc03).lookX == 0 && apply(0xc03).lookY == 0, "Opposite camera directions do not cancel");
    Require(apply(0x401).lookX == 70 && apply(0x401).lookY == 70, "Digital diagonal is not normalized");
    Require(apply(0x40).buttons == 1 && apply(4).buttons == 0x400, "ABXY jump/inventory defaults differ");
    Require(apply(0x300).buttons == 0x800 && apply(0x280).buttons == 2, "ABXY crafting/drop chord differs");
    Require(apply(0xc03).buttons == 0, "ABXY camera activated old face-button actions");
    Require(apply(1, ControlSettings::EContext::MENU).buttons == 1 && apply(1, ControlSettings::EContext::MENU).lookX == 0,
        "ABXY intercepted a menu action");
    ControlSettings::SetBinding(config, 0, 1);
    Require(apply(1).buttons == 1 && apply(1).lookX == 100, "Shared camera/game action was rejected");
    ControlSettings::SetBinding(config, ControlSettings::cameraBindingBegin + 3, 0);
    Require(apply(1).lookX == 0 && apply(1).buttons == 1, "Unbinding camera direction changed another action");
    ControlSettings::SetBinding(config, ControlSettings::cameraBindingBegin, 0x401);
    Require(apply(0x401).lookY == 100 && apply(0x400).lookY == 0, "Custom camera chord differs");
}

int main() {
    try {
        SettingsTests::CheckSettings();
        SettingsTests::CheckToggle();
        SettingsTests::CheckMigration();
        SettingsTests::CheckTouch();
        SettingsTests::CheckTextures();
        SettingsTests::CheckPrompts();
        SettingsTests::CheckAbxy();
        SettingsTests::CheckCameraAndPresets();
        std::cout << "Settings bounds, migration, toggle, touch, mip plans and prompt mapping passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
