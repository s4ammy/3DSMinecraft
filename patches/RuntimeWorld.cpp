#include "RuntimeWorld.h"

RuntimeWorld::TSettings &RuntimeWorld::Active() {
    //The bounded 49-entry table ends here, within the original 57-entry reservation.
    return *reinterpret_cast<TSettings *>(0xb16348);
}

void RuntimeWorld::BuildOffsets() {
    auto &settings = Active();
    auto *offset = reinterpret_cast<int *>(0xb161c0);
    const auto radius = static_cast<int>(settings.values[1]);
    const auto side = radius * 2 + 1;
    for (auto z = -radius; z <= radius; ++z) {
        for (auto x = -radius; x <= radius; ++x) {
            *offset++ = ((z + radius) & 1) != 0 ? -x : x;
            *offset++ = z;
        }
    }

    const auto stages = static_cast<unsigned int>(side * side * 2);
    settings.tableBytes = stages * 4;
    settings.readyThreshold = stages - stages / 10;
    settings.totalStages = static_cast<float>(stages);
}

void RuntimeWorld::InitializeWorldSettings() {
    auto &settings = Active();
    settings.values[0] = 1;
    settings.values[1] = 2;
    settings.values[2] = 24;
    settings.values[3] = 4;
    settings.initialized = 0;
    BuildOffsets();
}

void RuntimeWorld::Apply(const ControlSettings::TConfig &config) {
    auto &settings = Active();
    if (settings.initialized != 0) {
        return;
    }

    for (auto index = 0U; index < 4; ++index) {
        settings.values[index] = ControlSettings::GetSetting(config,
            static_cast<ControlSettings::ESetting>(static_cast<unsigned int>(ControlSettings::ESetting::RENDER_DISTANCE) + index));
    }

    BuildOffsets();
    settings.initialized = 1;
}

bool RuntimeWorld::NeedsRestart(const ControlSettings::TConfig &config) {
    for (auto index = 0U; index < 4; ++index) {
        if (Active().values[index] != ControlSettings::GetSetting(config,
            static_cast<ControlSettings::ESetting>(static_cast<unsigned int>(ControlSettings::ESetting::RENDER_DISTANCE) + index))) {
            return true;
        }
    }

    return false;
}
