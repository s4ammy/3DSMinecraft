#include "DebugOverlay.h"
#include "RuntimeControls.h"

namespace DebugOverlay {
    struct TColor {
        float red;
        float green;
        float blue;
        float alpha;
    };

    void DrawLine(void *font, const TTextBuffer &buffer, float y, const TColor &color);
    unsigned int GetMode();
}

static_assert(sizeof(DebugOverlay::TOverlayState) <= 112);
static_assert(__builtin_offsetof(DebugOverlay::TTextBuffer, text) == 12);

unsigned int DebugOverlay::GetMode() {
    auto *const *slot = reinterpret_cast<RuntimeControls::TState *const *>(0xb3fc30);
    const auto *controls = __atomic_load_n(slot, __ATOMIC_ACQUIRE);
    return controls == nullptr ? 0 : controls->config.globalSettings[0];
}

void DebugOverlay::DrawLine(void *font, const TTextBuffer &buffer, float y, const TColor &color) {
    using TDrawShadow = void (*)(void *, const char **, float, float, const TColor *, unsigned int);
    const auto *text = buffer.text;
    reinterpret_cast<TDrawShadow>(0x55b7bc)(font, &text, 4.0f, y, &color, 0);
}

void DebugOverlay::DrawDebugOverlay(void *, const void *minecraft) {
    const auto mode = GetMode();
    if (mode == 0 || minecraft == nullptr) {
        return;
    }

    auto *controls = *reinterpret_cast<RuntimeControls::TState *const *>(0xb3fc30);
    const auto *hud = controls == nullptr ? nullptr : static_cast<const unsigned char *>(controls->touch.hud);
    using TCurrentScreen = const void *(*)(const void *);
    if (hud == nullptr || *reinterpret_cast<const unsigned int *>(hud + 184) != 0 ||
        reinterpret_cast<TCurrentScreen>(0x12d544)(minecraft) != hud) {
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

    if (mode == 2 && snapshot.valid != 0 && state.readSequence != sequence) {
        state.readSequence = sequence;
        const auto *mainHeap = *reinterpret_cast<void *const *>(0xa30d18);
        const auto *graphicsHeap = *reinterpret_cast<void *const *>(0xa30d1c);
        const auto *streamingHeap = *reinterpret_cast<void *const *>(0xa30d24);
        state.heapValid = mainHeap != nullptr && graphicsHeap != nullptr && streamingHeap != nullptr;
        if (state.heapValid != 0) {
            using TReadHeaps = void (*)(unsigned int *, unsigned int *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
            reinterpret_cast<TReadHeaps>(0x1399a8)(&state.heapBytes[0], &state.heapBytes[1], &state.heapBytes[2], &state.heapBytes[3], &state.heapBytes[4], &state.heapBytes[5]);
#ifdef MC3DS_ADVANCED_OVERLAY
            const void *arenas[] = {mainHeap, graphicsHeap, streamingHeap};
            for (auto index = 0U; index < 3; ++index) {
                controls->diagnostics.largest[index] = ReadLargestBlock(arenas[index]);
            }
#endif
        }
    }

#ifdef MC3DS_OVERLAY_BACKGROUND
    const auto lineCount = mode == 1 ? 1U : 5U;
    TTextBuffer lines[6];
    FormatTimingLine(lines[0], snapshot, false);
    if (lineCount == 5) {
        FormatTimingLine(lines[1], snapshot, true);
        const char *labels[] = {"Main ", "Gfx ", "Stream "};
        for (auto index = 0U; index != 3; ++index) {
            FormatHeapLine(lines[index + 2], labels[index], state.heapBytes[index * 2], state.heapBytes[index * 2 + 1], state.heapValid != 0);
        }
    }

    auto count = lineCount;
    auto scale = 1.0f;
    auto opacity = 0.75f;
    auto corner = 0U;
    auto graph = false;
#ifdef MC3DS_ADVANCED_OVERLAY
    scale = ControlSettings::GetSetting(controls->config, ControlSettings::ESetting::OVERLAY_SCALE) * 0.01f;
    opacity = ControlSettings::GetSetting(controls->config, ControlSettings::ESetting::OVERLAY_OPACITY) * 0.01f;
    corner = ControlSettings::GetSetting(controls->config, ControlSettings::ESetting::OVERLAY_POSITION);
    graph = ControlSettings::GetSetting(controls->config, ControlSettings::ESetting::FRAME_GRAPH) != 0;
    if (mode == 2) {
        ResetText(lines[5]);
        AppendText(lines[5], "Largest M/G/S ");
        for (auto index = 0U; index < 3; ++index) {
            if (index != 0) {
                AppendText(lines[5], "/");
            }

            AppendTenths(lines[5], static_cast<unsigned int>(controls->diagnostics.largest[index] * (10.0f / 1048576.0f)));
        }

        AppendText(lines[5], " MiB");
        count = 6;
    }
#endif
    auto width = 0;
    using TMeasureText = int (*)(void *, const char **, unsigned int, float);
    for (auto index = 0U; index != count; ++index) {
        const auto *text = lines[index].text;
        const auto lineWidth = reinterpret_cast<TMeasureText>(0x71ad28)(font, &text, 0, scale);
        if (lineWidth > width) {
            width = lineWidth;
        }
    }

    if (width > 312) {
        scale *= 312.0f / width;
        width = 312;
    }

    if (graph && width < 96 * scale) {
        width = static_cast<int>(96 * scale);
    }

    const auto height = static_cast<int>((count * 14 + (graph ? 34 : 0)) * scale) + 3;
    const auto left = (corner & 1) != 0 ? 312 - width : 1;
    const auto top = (corner & 2) != 0 ? 239 - height : 1;
    const auto background = TColor{20.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, opacity};
    using TFillRectangle = void (*)(void *, int, int, int, int, const TColor *);
    reinterpret_cast<TFillRectangle>(0x6cff68)(nullptr, left, top, left + width + 7, top + height - 1, &background);
    const auto accent = TColor{0.73f, 0.73f, 1.0f, 1.0f};
    const auto white = TColor{1.0f, 1.0f, 1.0f, 1.0f};
    for (auto index = 0U; index != count; ++index) {
        const auto *text = lines[index].text;
#ifdef MC3DS_ADVANCED_OVERLAY
        DrawScaledText(font, &text, reinterpret_cast<const float *>(index == 0 ? &accent : &white),
            left + 3.0f, top + 3.0f + index * 14.0f * scale, scale);
#else
        DrawLine(font, lines[index], top + 3.0f + index * 14.0f, index == 0 ? accent : white);
#endif
    }
#ifdef MC3DS_ADVANCED_OVERLAY
    if (graph) {
        DrawFrameGraph(controls->diagnostics, left + 3.0f, top + 3.0f + count * 14.0f * scale, scale);
    }
#endif
#else
    const auto accent = TColor{0.73f, 0.73f, 1.0f, 1.0f};
    const auto white = TColor{1.0f, 1.0f, 1.0f, 1.0f};
    TTextBuffer buffer;
    FormatTimingLine(buffer, snapshot, false);
    DrawLine(font, buffer, 4.0f, accent);
    if (mode == 1) {
        return;
    }

    FormatTimingLine(buffer, snapshot, true);
    DrawLine(font, buffer, 18.0f, white);
    const char *labels[] = {"Main ", "Gfx ", "Stream "};
    for (auto index = 0U; index != 3; ++index) {
        FormatHeapLine(buffer, labels[index], state.heapBytes[index * 2], state.heapBytes[index * 2 + 1], state.heapValid != 0);
        DrawLine(font, buffer, 32.0f + static_cast<float>(index) * 14.0f, white);
    }
#endif
}

void *DebugOverlay::OnDisplayVblankWithOverlayStats(unsigned int display) {
    const auto *renderDevice = *reinterpret_cast<const volatile unsigned char *const *>(0xa35884);
    const auto newFrame = display == 0x400 && renderDevice != nullptr &&
        (renderDevice[388] == 2 || renderDevice[389] == 2 || renderDevice[390] == 2);
    using TVblank = void *(*)(unsigned int);
    auto *result = reinterpret_cast<TVblank>(0x4f9044)(display);
    if (newFrame) {
        auto &state = *reinterpret_cast<TOverlayState *>(0xac3818);
        if (GetMode() == 0) {
            state.frames.initialized = 0;
            return result;
        }

        unsigned long long tick;
        using TReadTick = void (*)(unsigned long long *);
        reinterpret_cast<TReadTick>(0x4cf728)(&tick);
#ifdef MC3DS_ADVANCED_OVERLAY
        auto *controls = *reinterpret_cast<RuntimeControls::TState *const *>(0xb3fc30);
        auto &diagnostics = controls->diagnostics;
        const auto interval = tick - state.frames.lastTick;
        if (ControlSettings::GetSetting(controls->config, ControlSettings::ESetting::FRAME_GRAPH) != 0 &&
            state.frames.initialized != 0 && tick >= state.frames.lastTick && interval != 0 && interval <= ticksPerSecond * 5ULL) {
            const auto cursor = __atomic_load_n(&diagnostics.cursor, __ATOMIC_RELAXED);
            const auto ms = static_cast<unsigned int>(static_cast<float>(static_cast<unsigned int>(interval)) * (1000.0f / ticksPerSecond) + 0.5f);
            __atomic_store_n(&diagnostics.intervals[cursor & 31], ms, __ATOMIC_RELAXED);
            __atomic_store_n(&diagnostics.cursor, cursor + 1, __ATOMIC_RELEASE);
        }
#endif
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
