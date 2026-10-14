#include "RuntimeVisuals.h"

bool RuntimeVisuals::PlanTerrainTexture(const unsigned int *header, unsigned int fileBytes, const char *name, TTexturePlan &plan) {
    const char *base = name;
    for (const auto *cursor = name; *cursor != 0; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            base = cursor + 1;
        }
    }

    const auto *prefix = "atlas.terrain.meta_";
    while (*prefix != 0) {
        if (*base++ != *prefix++) {
            return false;
        }
    }

    for (auto index = 0U; index < 8; ++index) {
        const auto digit = *base++;
        if (!((digit >= '0' && digit <= '9') || (digit >= 'A' && digit <= 'F'))) {
            return false;
        }
    }

    if (*base++ != '_' || *base < '0' || *base > '9') {
        return false;
    }

    while (*base >= '0' && *base <= '9') {
        ++base;
    }

    const auto *suffix = ".3dst";
    while (*suffix != 0) {
        if (*base++ != *suffix++) {
            return false;
        }
    }

    const auto width = header[3];
    const auto height = header[4];
    if (*base != 0 || header[0] != 0x54534433 || header[1] != 3 || (header[2] != 0 && header[2] != 4) ||
        width != header[5] || height != header[6] || header[7] != 3 ||
        width < 32 || height < 32 || width > 2048 || height > 2048 ||
        (width & (width - 1)) != 0 || (height & (height - 1)) != 0) {
        return false;
    }

    const auto baseBytes = width * height * (header[2] == 0 ? 4U : 2U);
    const auto payloadBytes = (baseBytes >> 2) + (baseBytes >> 4);
    if (fileBytes != 32 + baseBytes + payloadBytes) {
        return false;
    }

    plan = {baseBytes, payloadBytes};
    return true;
}
