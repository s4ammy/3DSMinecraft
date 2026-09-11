#pragma once

namespace Performance {
    struct TBlockPos {
        int x;
        int y;
        int z;
    };

    struct TChunkPos {
        int x;
        int z;
    };

    struct TChunk {
        unsigned char prefix[32];
        TChunkPos position;
        unsigned char middle[52];
        const unsigned char *subChunks[8];
        unsigned int subChunkCount;
    };

    struct TBlockSource {
        unsigned char prefix[24];
        short height;
        unsigned char middle[26];
        const TChunk *cachedChunk;
    };

    unsigned int GetBlockId(unsigned char *outputBlockId, TBlockSource *blockSource, const TBlockPos *blockPos);
}
