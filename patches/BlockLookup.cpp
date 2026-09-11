#include "BlockLookup.h"

static_assert(__builtin_offsetof(Performance::TBlockSource, cachedChunk) == 52);
static_assert(__builtin_offsetof(Performance::TChunk, subChunks) == 92);
static_assert(__builtin_offsetof(Performance::TChunk, subChunkCount) == 124);

unsigned int Performance::GetBlockId(unsigned char *outputBlockId, TBlockSource *blockSource, const TBlockPos *blockPos) {
    const auto y = blockPos->y;
    if (y >= 0 && y < blockSource->height) {
        const auto position = TChunkPos{blockPos->x >> 4, blockPos->z >> 4};
        const auto *chunk = blockSource->cachedChunk;
        if (chunk == nullptr || chunk->position.x != position.x || chunk->position.z != position.z) {
            using TGetChunk = const TChunk *(*)(TBlockSource *, const TChunkPos *);
            const auto getChunk = reinterpret_cast<TGetChunk>(0x17ab3c);
            chunk = getChunk(blockSource, &position);
        }

        const auto subChunkIndex = static_cast<unsigned int>(y) >> 4;
        if (chunk != nullptr && subChunkIndex < chunk->subChunkCount) {
            const auto *subChunk = chunk->subChunks[subChunkIndex];
            if (subChunk != nullptr) {
                const auto index = ((static_cast<unsigned int>(blockPos->x) & 15U) << 8) |
                    ((static_cast<unsigned int>(blockPos->z) & 15U) << 4) |
                    (static_cast<unsigned int>(y) & 15U);
                const auto blockId = subChunk[index];
                *outputBlockId = blockId;
                return blockId;
            }
        }
    }

    const auto airId = *reinterpret_cast<const unsigned char *>(0xa34724);
    *outputBlockId = airId;
    return airId;
}
