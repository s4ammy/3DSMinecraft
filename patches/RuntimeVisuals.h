#pragma once

namespace RuntimeVisuals {
    struct TTexturePlan {
        unsigned int skippedBytes;
        unsigned int payloadBytes;
    };

    bool PlanTerrainTexture(const unsigned int *header, unsigned int fileBytes, const char *name, TTexturePlan &plan);
    extern "C" void RenderRuntimeParticles(void *engine, const void *first, const void *second, float partialTick, float distance);
    extern "C" bool RuntimeWeatherEnabled();
    extern "C" bool RuntimeMapRefresh(unsigned char *hud);
    extern "C" unsigned int RuntimeTextureLength(unsigned int *header, unsigned int *stream, unsigned int fileBytes, const char *name);
}
