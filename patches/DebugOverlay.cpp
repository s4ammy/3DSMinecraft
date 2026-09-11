#include "DebugOverlay.h"

namespace DebugOverlay {
    struct TColor {
        float red;
        float green;
        float blue;
        float alpha;
    };

    void DrawLine(void *font, const TTextBuffer &buffer, float y, const TColor &color);
}

static_assert(sizeof(DebugOverlay::TOverlayState) <= 112);
static_assert(__builtin_offsetof(DebugOverlay::TTextBuffer, text) == 12);

void DebugOverlay::DrawLine(void *font, const TTextBuffer &buffer, float y, const TColor &color) {
    using TDrawShadow = void (*)(void *, const char **, float, float, const TColor *, unsigned int);
    const auto *text = buffer.text;
    reinterpret_cast<TDrawShadow>(0x55b7bc)(font, &text, 4.0f, y, &color, 0);
}

void DebugOverlay::DrawDebugOverlay(void *, const void *minecraft) {
    if (minecraft == nullptr) {
        return;
    }

    const auto *minecraftBytes = static_cast<const unsigned char *>(minecraft);
    auto *font = *reinterpret_cast<void *const *>(minecraftBytes + 92);
    if (font == nullptr) {
        return;
    }

    auto &state = *reinterpret_cast<TOverlayState *>(0xac3818);
    const auto sequence = __atomic_load_n(&state.sequence, __ATOMIC_ACQUIRE);
    if ((sequence & 1U) != 0) {
        return;
    }

    TFrameStats snapshot;
    snapshot.fpsTenths = __atomic_load_n(&state.published[0], __ATOMIC_RELAXED);
    snapshot.averageMsTenths = __atomic_load_n(&state.published[1], __ATOMIC_RELAXED);
    snapshot.peakMsTenths = __atomic_load_n(&state.published[2], __ATOMIC_RELAXED);
    snapshot.valid = __atomic_load_n(&state.published[3], __ATOMIC_RELAXED);
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    if (sequence != __atomic_load_n(&state.sequence, __ATOMIC_RELAXED)) {
        return;
    }

    if (snapshot.valid != 0 && state.readSequence != sequence) {
        state.readSequence = sequence;
        const auto *mainHeap = *reinterpret_cast<void *const *>(0xa30d18);
        const auto *graphicsHeap = *reinterpret_cast<void *const *>(0xa30d1c);
        const auto *streamingHeap = *reinterpret_cast<void *const *>(0xa30d24);
        state.heapValid = mainHeap != nullptr && graphicsHeap != nullptr && streamingHeap != nullptr;
        if (state.heapValid != 0) {
            using TReadHeaps = void (*)(unsigned int *, unsigned int *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
            reinterpret_cast<TReadHeaps>(0x1399a8)(&state.heapBytes[0], &state.heapBytes[1], &state.heapBytes[2], &state.heapBytes[3], &state.heapBytes[4], &state.heapBytes[5]);
        }
    }

    const auto accent = TColor{0.73f, 0.73f, 1.0f, 1.0f};
    const auto white = TColor{1.0f, 1.0f, 1.0f, 1.0f};
    TTextBuffer buffer;
    FormatTimingLine(buffer, snapshot, false);
    DrawLine(font, buffer, 80.0f, accent);
    FormatTimingLine(buffer, snapshot, true);
    DrawLine(font, buffer, 94.0f, white);
    const char *labels[] = {"Main ", "Gfx ", "Stream "};
    for (auto index = 0U; index != 3; ++index) {
        FormatHeapLine(buffer, labels[index], state.heapBytes[index * 2], state.heapBytes[index * 2 + 1], state.heapValid != 0);
        DrawLine(font, buffer, 108.0f + static_cast<float>(index) * 14.0f, white);
    }
}

void *DebugOverlay::OnDisplayVblankWithOverlayStats(unsigned int display) {
    const auto *renderDevice = *reinterpret_cast<const volatile unsigned char *const *>(0xa35884);
    const auto newFrame = display == 0x400 && renderDevice != nullptr &&
        (renderDevice[388] == 2 || renderDevice[389] == 2 || renderDevice[390] == 2);
    using TVblank = void *(*)(unsigned int);
    auto *result = reinterpret_cast<TVblank>(0x4f9044)(display);
    if (newFrame) {
        auto &state = *reinterpret_cast<TOverlayState *>(0xac3818);
        unsigned long long tick;
        using TReadTick = void (*)(unsigned long long *);
        reinterpret_cast<TReadTick>(0x4cf728)(&tick);
        if (UpdateFrameStats(state.frames, tick) || state.frames.valid == 0) {
            const auto sequence = __atomic_load_n(&state.sequence, __ATOMIC_RELAXED);
            __atomic_store_n(&state.sequence, sequence + 1, __ATOMIC_SEQ_CST);
            __atomic_store_n(&state.published[0], state.frames.fpsTenths, __ATOMIC_RELAXED);
            __atomic_store_n(&state.published[1], state.frames.averageMsTenths, __ATOMIC_RELAXED);
            __atomic_store_n(&state.published[2], state.frames.peakMsTenths, __ATOMIC_RELAXED);
            __atomic_store_n(&state.published[3], state.frames.valid, __ATOMIC_RELAXED);
            __atomic_store_n(&state.sequence, sequence + 2, __ATOMIC_RELEASE);
        }
    }

    return result;
}
