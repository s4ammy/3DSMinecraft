#pragma once

namespace TouchStick {
    inline constexpr int centerX = 104;
    inline constexpr int centerY = 160;
    inline constexpr int radius = 52;

    struct TSettings {
        int x = centerX;
        int y = centerY;
        int radius = TouchStick::radius;
        int deadZone = 7;
    };

    struct TInputState {
        short lookX;
        short lookY;
        bool wasHeld;
        bool ownsTouch;
        bool canceled;
    };

    struct TState {
        void *hud;
        TInputState input;
    };

    bool Update(TInputState &state, int x, int y, bool held, bool gameplay);
    bool UpdateConfigured(TInputState &state, int x, int y, bool held, bool gameplay, const TSettings &settings);
    bool IsGameplay(void *hud);
    void DrawDisk(float x, float y, float radius, const float *color);
    extern "C" void UpdateTouchStick(unsigned char *inputManager, unsigned int controller);
    extern "C" void DrawTouchStick(void *hud, int x, int y, float partialTick);
}
