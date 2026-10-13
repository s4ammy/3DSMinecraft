#pragma once

#include "ControlSettings.h"
#include "TouchStick.h"

namespace RuntimeControls {
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
        bool editMenus;
        unsigned char reserved;
        volatile ControlSettings::EMode workerMode;
        char status[96];
        unsigned char workerStack[4104];
    };

    static_assert(sizeof(TState) == 4576, "Runtime controls state ABI changed");
    static_assert(__builtin_offsetof(TState, workerContext) == 16, "Thread entry context ABI changed");
    static_assert(__builtin_offsetof(TState, workerStack) == 472, "Worker stack layout changed");

    TState *GetState();
    void RefreshLabels(TState &state);
    void Changed(TState &state);
    bool IsControlsPage(const TState &state);
    void Capture(TState &state, unsigned int buttons, bool touching);
    char *Append(char *text, const char *value);
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
