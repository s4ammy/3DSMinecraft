#include "RuntimeVisuals.h"
#include "RuntimeControls.h"
#include "DebugOverlay.h"

extern "C" {
extern RuntimeControls::TState *runtimeControls;
void ControlsOriginalParticles(void *engine, const void *first, const void *second, float partialTick, float distance);
void ControlsReadTick(unsigned long long *tick);
}

void RuntimeVisuals::RenderRuntimeParticles(void *engine, const void *first, const void *second, float partialTick, float distance) {
    const auto *state = runtimeControls;
    const auto setting = state == nullptr ? 0 : ControlSettings::GetSetting(state->config, ControlSettings::ESetting::PARTICLES);
    if (setting != 2) {
        ControlsOriginalParticles(engine, first, second, partialTick, setting == 1 ? distance * 0.5f : distance);
    }
}

bool RuntimeVisuals::RuntimeWeatherEnabled() {
    const auto *state = runtimeControls;
    return state == nullptr || ControlSettings::GetSetting(state->config, ControlSettings::ESetting::WEATHER) == 0;
}

bool RuntimeVisuals::RuntimeMapRefresh(unsigned char *hud) {
    auto *state = runtimeControls;
    if (state == nullptr) {
        return true;
    }

    const auto rate = ControlSettings::GetSetting(state->config, ControlSettings::ESetting::MAP_RATE);
    if (rate == 0) {
        return true;
    }

    unsigned long long tick;
    ControlsReadTick(&tick);
    const auto index = *reinterpret_cast<const unsigned int *>(hud + 492);
    const auto needsInit = index >= 32 || *reinterpret_cast<void **>(hud + 216 + index * 4) == nullptr;
    const auto first = state->mapOwner != hud || state->mapRate != rate || needsInit;
    const auto interval = rate == 1 ? DebugOverlay::ticksPerSecond / 4 : DebugOverlay::ticksPerSecond;
    if (!first && (rate == 3 || (tick >= state->mapTick && tick - state->mapTick < interval))) {
        return false;
    }

    state->mapOwner = hud;
    state->mapRate = rate;
    state->mapTick = tick;
    return true;
}

unsigned int RuntimeVisuals::RuntimeTextureLength(unsigned int *header, unsigned int *stream, unsigned int fileBytes, const char *name) {
    auto *state = runtimeControls;
    TTexturePlan plan;
    if (state == nullptr || state->activeLowMemoryTextures == 0 || stream[2] != 32 || stream[3] != 0 ||
        !PlanTerrainTexture(header, fileBytes, name, plan)) {
        return fileBytes;
    }

    //Archive reads use an explicit 64-bit position, not an OS seek cursor.
    stream[2] += plan.skippedBytes;
    for (auto index = 3U; index <= 6; ++index) {
        header[index] >>= 1;
    }

    --header[7];
    ++state->textureLoads;
    state->textureBytesSaved += plan.skippedBytes;
    return 32 + plan.payloadBytes;
}
