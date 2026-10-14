#pragma once

namespace Performance {
    struct TLightSubChunk {
        unsigned char storage[6144];
        const unsigned char *lightBytes;
    };

    struct TLightChunk {
        unsigned char prefix[92];
        const TLightSubChunk *subChunks[8];
        unsigned int subChunkCount;
        unsigned char remaining[2884 - 128];
        unsigned char defaultSkyLight;
        unsigned char defaultBlockLight;
    };

    struct TLightPosition {
        unsigned char x;
        unsigned char z;
        short y;
    };

    extern "C" unsigned int GetChunkLight(unsigned char *output, const TLightChunk *chunk, const TLightPosition *position);
}
