#pragma once

#include "controls/ControlSettings.h"
#include "controls/TouchStick.h"
#include "overlay/DebugOverlay.h"

namespace RuntimeControls {
    enum class EPage : unsigned char {
        GAMEPLAY,
        MENUS,
        CAMERA,
        TOUCH,
        DISPLAY,
        TOOLS,
        OVERLAY,
        COUNT
    };
    struct TString {
        char *data;
    };

    struct TReference {
        unsigned char *object;
        void *control;
    };

    struct TPair {
        TString key;
        TString value;
    };

    struct TPairs {
        TPair *begin;
        TPair *end;
        TPair *capacity;
    };

    struct TState {
        unsigned int threadHandle;
        volatile unsigned int stopWorker;
        unsigned int workerStarted;
        int workerResult;
        unsigned int workerContext[3];
        ControlSettings::TConfig config;
        ControlSettings::TInputState inputs[4];
        TouchStick::TState touch;
        unsigned char *screen;
        unsigned char *options;
        unsigned char *buttons[21];
        unsigned int rawButtons;
        unsigned int captureButtons;
        int captureIndex;
        bool captureReady;
        EPage page;
        ControlSettings::EMode copySource;
        volatile ControlSettings::EMode workerMode;
        char status[96];
        unsigned int selectedBinding;
        void *mapOwner;
        unsigned long long mapTick;
        unsigned int mapRate;
        unsigned int textureLoads;
        unsigned int textureBytesSaved;
        unsigned int activeLowMemoryTextures;
#ifdef MC3DS_NATIVE_MENU
        void *nativeMenu;
#endif
        unsigned char workerStack[4104];
#ifdef MC3DS_NATIVE_MENU
        DebugOverlay::TDiagnostics diagnostics;
#endif
    };

    static_assert(sizeof(TState) <= 5120, "Runtime controls state exceeded its budget");
    static_assert(__builtin_offsetof(TState, workerContext) == 16, "Thread entry context ABI changed");
    static_assert(sizeof(TState::workerStack) >= 4096 + 7, "Worker stack alignment margin changed");

    TState *GetState();
    void RefreshLabels(TState &state);
    unsigned int RowBinding(const TState &state, unsigned int row);
    void Changed(TState &state);
    bool IsControlsPage(const TState &state);
    void Capture(TState &state, unsigned int buttons, bool touching);
    char *Append(char *text, const char *value);
    char *AppendNumber(char *text, unsigned int value);
    void DescribeSetting(const TState &state, ControlSettings::ESetting setting, char *text);
    void DescribePageRow(const TState &state, unsigned int row, char *text);
    void ActivatePageRow(TState &state, unsigned int row);
    TouchStick::TSettings GetTouchSettings(const TState &state);
    bool Equals(const char *left, const char *right);

    extern "C" void BuildControlRows(unsigned char *screen);
    extern "C" int HandleControlButton(unsigned char *screen, unsigned int button);
    extern "C" int DispatchControlRelease(unsigned char *screen, int action);
    extern "C" void DestroyControlRows(unsigned char *screen);
    extern "C" void UpdateRuntimeControls(unsigned char *inputManager, unsigned int controller);
    extern "C" void DrawRuntimeControls(void *hud, int x, int y, float partialTick);
    extern "C" void ForgetRuntimeHud(void *hud);
    extern "C" void LoadControlSettings(TPairs *pairs, unsigned char *options);
    extern "C" int SaveControlSettings(unsigned char *options, TPairs *pairs);
    extern "C" void StartControlAccessory();
    extern "C" void ControlAccessoryWorker(TState *state);
    extern "C" void StopControlAccessory();
}
