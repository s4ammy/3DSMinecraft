#pragma once

#include "ControlSettings.h"

namespace RuntimeWorld {
    struct TSettings {
        unsigned int values[4];
        unsigned int tableBytes;
        unsigned int readyThreshold;
        float totalStages;
        unsigned int initialized;
    };

    static_assert(sizeof(TSettings) == 32, "World settings exceed the unused chunk-table tail");
    static_assert(__builtin_offsetof(TSettings, tableBytes) == 16 &&
        __builtin_offsetof(TSettings, readyThreshold) == 20 &&
        __builtin_offsetof(TSettings, totalStages) == 24, "World hook field offsets changed");

    TSettings &Active();
    void BuildOffsets();
    void Apply(const ControlSettings::TConfig &config);
    bool NeedsRestart(const ControlSettings::TConfig &config);
    extern "C" void InitializeWorldSettings();
}
