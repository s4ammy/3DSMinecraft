#include "controls/TouchStick.h"

extern "C" {
    extern TouchStick::TState touchStickState;
    extern unsigned char touchTessellator[];
    extern unsigned char touchGuiMaterial[];
    extern float touchGuiColorState[];
    extern float touchWhiteColor[];
    void *TouchGetCurrentScreen(void *client);
    void TouchRenderBottomHud(void *hud, int x, int y, float partialTick);
    void TouchSetGuiColor(void *state, const float *color);
    void TouchBegin(void *tessellator, int vertices);
    void TouchColor(void *tessellator, const float *color);
    void TouchVertex(void *tessellator, float x, float y, float z);
    void TouchDraw(void *tessellator, void *material);
}

static_assert(sizeof(TouchStick::TState) <= 16);

bool TouchStick::IsGameplay(void *hud) {
    if (hud == nullptr) {
        return false;
    }

    const auto *bytes = static_cast<const unsigned char *>(hud);
    return TouchGetCurrentScreen(*reinterpret_cast<void *const *>(bytes + 4)) == hud &&
        *reinterpret_cast<const unsigned int *>(bytes + 184) == 0;
}

void TouchStick::UpdateTouchStick(unsigned char *inputManager, unsigned int controller) {
    const auto *coordinates = reinterpret_cast<const short *>(inputManager + 1160);
    const auto consumed = Update(touchStickState.input, coordinates[0], coordinates[1], inputManager[1164] != 0,
        IsGameplay(touchStickState.hud));
    //Physical input leaves the final slot active. Avoid queuing touch in inactive slots.
    if (controller == 3) {
        auto *camera = reinterpret_cast<short *>(inputManager + 356);
        camera[0] = touchStickState.input.lookX;
        camera[1] = touchStickState.input.lookY;
    }

    if (consumed) {
        //Every controller samples the same touchscreen. Consume all slots and the release.
        inputManager[1164] = 0;
        inputManager[1172] = 0;
    }
}

void TouchStick::DrawDisk(float x, float y, float diskRadius, const float *color) {
    TouchColor(touchTessellator, color);
    auto dx = diskRadius;
    auto dy = 0.0f;
    for (auto index = 0; index < 24; ++index) {
        const auto nextX = dx * 0.965925826f + dy * 0.258819045f;
        const auto nextY = dy * 0.965925826f - dx * 0.258819045f;
        TouchVertex(touchTessellator, x, y, 0.0f);
        TouchVertex(touchTessellator, x + dx, y + dy, 0.0f);
        TouchVertex(touchTessellator, x + nextX, y + nextY, 0.0f);
        TouchVertex(touchTessellator, x, y, 0.0f);
        dx = nextX;
        dy = nextY;
    }
}

void TouchStick::DrawTouchStick(void *hud, int x, int y, float partialTick) {
    TouchRenderBottomHud(hud, x, y, partialTick);
    touchStickState.hud = hud;
    if (!IsGameplay(hud)) {
        return;
    }

    const float rim[] = {0.82f, 0.88f, 0.92f, 0.65f};
    const float inside[] = {0.08f, 0.12f, 0.18f, 0.42f};
    const float knob[] = {0.9f, 0.95f, 1.0f, 0.85f};
    float previousColor[4];
    for (auto index = 0; index < 4; ++index) {
        previousColor[index] = touchGuiColorState[index];
    }

    TouchSetGuiColor(touchGuiColorState, touchWhiteColor);
    TouchBegin(touchTessellator, 288);
    DrawDisk(centerX, centerY, radius, rim);
    DrawDisk(centerX, centerY, radius - 2, inside);
    DrawDisk(centerX + touchStickState.input.lookX * 0.4f, centerY - touchStickState.input.lookY * 0.4f, 10.0f, knob);
    TouchDraw(touchTessellator, touchGuiMaterial);
    TouchSetGuiColor(touchGuiColorState, previousColor);
}
