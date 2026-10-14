#include "overlay/DebugOverlay.h"

extern "C" {
void NativeLockArena(void *mutex);
void NativeUnlockArena(void *mutex);
void TouchSetGuiColor(void *state, const float *color);
void TouchBegin(void *tessellator, int vertices);
void TouchColor(void *tessellator, const float *color);
void TouchVertex(void *tessellator, float x, float y, float z);
void TouchDraw(void *tessellator, void *material);
extern unsigned char touchTessellator[];
extern unsigned char touchGuiMaterial[];
extern float touchGuiColorState[];
extern float touchWhiteColor[];
}

unsigned int DebugOverlay::ReadLargestBlock(const void *arena) {
    auto *mutex = reinterpret_cast<unsigned char *>(const_cast<void *>(arena)) + 88;
    NativeLockArena(mutex);
    auto largest = 0U;
    auto *node = *reinterpret_cast<const unsigned int *const *>(static_cast<const unsigned char *>(arena) + 60);
    while (node != nullptr) {
        if (node[1] > largest) {
            largest = node[1];
        }

        node = reinterpret_cast<const unsigned int *>(node[3]);
    }

    NativeUnlockArena(mutex);
    return largest;
}

void DebugOverlay::DrawScaledText(void *font, const char **text, const float *color, float x, float y, float scale) {
    unsigned int matrixReference[2];
    using TPush = void (*)(unsigned int *, void *);
    using TMatrix = void *(*)(unsigned int *);
    using TTransform = void (*)(void *, float, float, float);
    using TRelease = void (*)(unsigned int *);
    using TDraw = void (*)(void *, const char **, const float *, unsigned int, unsigned int, float, float, float);
    reinterpret_cast<TPush>(0x19b9dc)(matrixReference, *reinterpret_cast<void **>(0xa358b8));
    auto *matrix = reinterpret_cast<TMatrix>(0x19b7a8)(matrixReference);
    reinterpret_cast<TTransform>(0x5fc550)(matrix, x, y, 0.0f);
    reinterpret_cast<TTransform>(0x5fc1f8)(matrix, scale, scale, 1.0f);
    //S2 is shadow offset, not font scale. Transform without disabling the native font cache.
    reinterpret_cast<TDraw>(0x55bb84)(font, text, color, 1, 0, 0.0f, 0.0f, 1.0f);
    reinterpret_cast<TRelease>(0x19b5c4)(matrixReference);
}

void DebugOverlay::DrawFrameGraph(const TDiagnostics &diagnostics, float x, float y, float scale) {
    float previous[4];
    for (auto index = 0; index < 4; ++index) {
        previous[index] = touchGuiColorState[index];
    }

    const auto cursor = __atomic_load_n(&diagnostics.cursor, __ATOMIC_ACQUIRE);
    const float normal[] = {0.55f, 0.9f, 0.55f, 0.9f};
    const float slow[] = {1.0f, 0.45f, 0.25f, 0.9f};
    const float guide[] = {0.7f, 0.7f, 1.0f, 0.8f};
    TouchSetGuiColor(touchGuiColorState, touchWhiteColor);
    TouchBegin(touchTessellator, 132);
    for (auto index = 0U; index < 33; ++index) {
        const auto interval = __atomic_load_n(&diagnostics.intervals[(cursor + index) & 31], __ATOMIC_RELAXED);
        const auto height = (interval > 100 ? 100 : interval) * 0.3f;
        const auto left = x + index * 3.0f * scale;
        const auto top = y + (index == 32 ? 20.0f : 30.0f - height) * scale;
        const auto right = index == 32 ? x + 96.0f * scale : left + 2.0f * scale;
        const auto bottom = index == 32 ? top + scale : y + 30.0f * scale;
        TouchColor(touchTessellator, index == 32 ? guide : interval > 34 ? slow : normal);
        TouchVertex(touchTessellator, index == 32 ? x : left, bottom, 0.0f);
        TouchVertex(touchTessellator, right, bottom, 0.0f);
        TouchVertex(touchTessellator, right, top, 0.0f);
        TouchVertex(touchTessellator, index == 32 ? x : left, top, 0.0f);
    }

    TouchDraw(touchTessellator, touchGuiMaterial);
    TouchSetGuiColor(touchGuiColorState, previous);
}
