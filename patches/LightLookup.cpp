#include "LightLookup.h"

static_assert(__builtin_offsetof(Performance::TLightSubChunk, lightBytes) == 6144);
static_assert(__builtin_offsetof(Performance::TLightChunk, subChunkCount) == 124);
static_assert(__builtin_offsetof(Performance::TLightChunk, defaultSkyLight) == 2884);
static_assert(sizeof(Performance::TLightPosition) == 4);

unsigned int Performance::GetChunkLight(unsigned char *output, const TLightChunk *chunk, const TLightPosition *position) {
    const auto section = static_cast<unsigned int>(static_cast<int>(position->y) >> 4);
    if (section >= chunk->subChunkCount) {
        output[0] = chunk->defaultSkyLight;
        output[1] = chunk->defaultBlockLight;
        return reinterpret_cast<unsigned int>(output);
    }

    auto index = (static_cast<unsigned int>(position->x) << 8) |
        (static_cast<unsigned int>(position->z) << 4) |
        (static_cast<unsigned int>(position->y) & 15U);
    if (position->x >= 16 || position->z >= 16) {
        //Retain the original assertion and index result if its handler returns.
        const unsigned char localPosition[] = {position->x, static_cast<unsigned char>(position->y & 15), position->z};
        using TGetIndex = unsigned int (*)(const unsigned char *);
        index = reinterpret_cast<TGetIndex>(0x6e3d2c)(localPosition);
    }

    const auto *lightBytes = chunk->subChunks[section]->lightBytes;
    const auto packedLight = lightBytes != nullptr ? lightBytes[index] : *reinterpret_cast<const unsigned char *>(0x989bb8);
    output[0] = packedLight >> 4;
    output[1] = packedLight & 15U;
    return packedLight & 15U;
}
