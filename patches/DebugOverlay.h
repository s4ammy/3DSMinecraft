#pragma once

namespace DebugOverlay {
    inline constexpr unsigned int ticksPerSecond = 16756991U * 16U;

    struct TFrameStats {
        unsigned long long lastTick;
        unsigned long long windowStart;
        unsigned int windowFrames;
        unsigned int windowPeakTicks;
        unsigned int fpsTenths;
        unsigned int averageMsTenths;
        unsigned int peakMsTenths;
        unsigned int initialized;
        unsigned int valid;
    };

    struct TOverlayState {
        TFrameStats frames;
        unsigned int heapBytes[6];
        unsigned int heapValid;
        unsigned int readSequence;
        unsigned int published[4];
        unsigned int sequence;
    };

    struct TTextBuffer {
        int references;
        unsigned int capacity;
        unsigned int length;
        char text[64];
    };

    bool UpdateFrameStats(TFrameStats &stats, unsigned long long tick);
    void ResetText(TTextBuffer &buffer);
    void AppendText(TTextBuffer &buffer, const char *text);
    void AppendTenths(TTextBuffer &buffer, unsigned int value);
    void FormatTimingLine(TTextBuffer &buffer, const TFrameStats &stats, bool peak);
    void FormatHeapLine(TTextBuffer &buffer, const char *label, unsigned int freeBytes, unsigned int usedBytes, bool valid);
    extern "C" void DrawDebugOverlay(void *display, const void *minecraft);
    extern "C" void *OnDisplayVblankWithOverlayStats(unsigned int display);
}
