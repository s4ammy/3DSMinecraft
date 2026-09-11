#include "DebugOverlay.h"

bool DebugOverlay::UpdateFrameStats(TFrameStats &stats, unsigned long long tick) {
    const auto interval = tick - stats.lastTick;
    if (stats.initialized == 0 || tick < stats.lastTick || interval > ticksPerSecond * 5ULL) {
        stats.lastTick = tick;
        stats.windowStart = tick;
        stats.windowFrames = 0;
        stats.windowPeakTicks = 0;
        stats.fpsTenths = 0;
        stats.averageMsTenths = 0;
        stats.peakMsTenths = 0;
        stats.initialized = 1;
        stats.valid = 0;
        return false;
    }

    if (interval == 0) {
        return false;
    }

    stats.lastTick = tick;
    ++stats.windowFrames;
    if (interval > stats.windowPeakTicks) {
        stats.windowPeakTicks = static_cast<unsigned int>(interval);
    }

    const auto elapsed = tick - stats.windowStart;
    if (elapsed < ticksPerSecond) {
        return false;
    }

    const auto elapsedTicks = static_cast<float>(static_cast<unsigned int>(elapsed));
    const auto frameCount = static_cast<float>(stats.windowFrames);
    const auto rate = frameCount * (10.0f * ticksPerSecond) / elapsedTicks;
    stats.fpsTenths = static_cast<unsigned int>((rate > 9999.0f ? 9999.0f : rate) + 0.5f);
    stats.averageMsTenths = static_cast<unsigned int>(elapsedTicks / frameCount * (10000.0f / ticksPerSecond) + 0.5f);
    stats.peakMsTenths = static_cast<unsigned int>(static_cast<float>(stats.windowPeakTicks) * (10000.0f / ticksPerSecond) + 0.5f);
    stats.valid = 1;
    stats.windowFrames = 0;
    stats.windowPeakTicks = 0;
    stats.windowStart = tick;
    return true;
}

void DebugOverlay::ResetText(TTextBuffer &buffer) {
    //A negative reference count makes the game's string cache copy this buffer.
    buffer.references = -1;
    buffer.capacity = sizeof(buffer.text) - 1;
    buffer.length = 0;
    buffer.text[0] = '\0';
}

void DebugOverlay::AppendText(TTextBuffer &buffer, const char *text) {
    while (*text != '\0' && buffer.length < sizeof(buffer.text) - 1) {
        buffer.text[buffer.length++] = *text++;
    }

    buffer.text[buffer.length] = '\0';
}

void DebugOverlay::AppendTenths(TTextBuffer &buffer, unsigned int value) {
    char reversed[10];
    auto count = 0U;
    auto whole = value / 10;
    do {
        reversed[count++] = static_cast<char>('0' + whole % 10);
        whole /= 10;
    } while (whole != 0);

    while (count != 0) {
        const char digit[] = {reversed[--count], '\0'};
        AppendText(buffer, digit);
    }

    const char fraction[] = {'.', static_cast<char>('0' + value % 10), '\0'};
    AppendText(buffer, fraction);
}

void DebugOverlay::FormatTimingLine(TTextBuffer &buffer, const TFrameStats &stats, bool peak) {
    ResetText(buffer);
    AppendText(buffer, peak ? "Peak " : "FPS ");
    if (stats.valid == 0) {
        AppendText(buffer, "--");
        return;
    }

    AppendTenths(buffer, peak ? stats.peakMsTenths : stats.fpsTenths);
    if (!peak) {
        AppendText(buffer, " | ");
        AppendTenths(buffer, stats.averageMsTenths);
    }

    AppendText(buffer, " ms");
}

void DebugOverlay::FormatHeapLine(TTextBuffer &buffer, const char *label, unsigned int freeBytes, unsigned int usedBytes, bool valid) {
    ResetText(buffer);
    AppendText(buffer, label);
    if (!valid || usedBytes > 0xffffffffU - freeBytes) {
        AppendText(buffer, "--");
        return;
    }

    const auto totalBytes = freeBytes + usedBytes;
    AppendTenths(buffer, static_cast<unsigned int>(static_cast<float>(usedBytes) * (10.0f / 1048576.0f)));
    AppendText(buffer, " / ");
    AppendTenths(buffer, static_cast<unsigned int>(static_cast<float>(totalBytes) * (10.0f / 1048576.0f)));
    AppendText(buffer, " MiB");
}
