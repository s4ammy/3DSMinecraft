#pragma once

#include "controls/RuntimeControls.h"

namespace NativeMenus {
    bool UpdateRow(RuntimeControls::TState &state, unsigned int row, const char *label);
    void Poll(RuntimeControls::TState &state);
    void ForgetScreen(unsigned char *screen);
    extern "C" void NativeInitializeMainMenu(unsigned char *screen);
    extern "C" void NativeInitializePauseMenu(unsigned char *screen);
    extern "C" void NativeInitializeOptions(unsigned char *screen);
    extern "C" void NativeOptionsBack(unsigned char *screen);
    extern "C" int NativeMainButtonEvent(unsigned char *screen, unsigned char *button);
    extern "C" int NativePauseButtonEvent(unsigned char *screen, unsigned char *button);
    extern "C" int NativeDropdownKey(unsigned char *widget, void *client, int action);
    extern "C" int NativeOptionsNavigation(unsigned char *screen, int event, int direction);
    extern "C" void NativeRenderOptions(unsigned char *screen, int x, int y, unsigned int flags);
}
