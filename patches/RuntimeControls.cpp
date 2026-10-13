#include "RuntimeControls.h"

extern "C" {
    extern RuntimeControls::TState *runtimeControls;
    void *ControlsAllocate(unsigned int size);
    void ControlsStringConstruct(RuntimeControls::TString *text, const char *value, void *temporary);
    void ControlsStringRelease(RuntimeControls::TString *text);
    void ControlsStringAssign(RuntimeControls::TString *text, const char *value);
    void ControlsUpdateButtonLabel(void *button);
    void ControlsCreateButton(RuntimeControls::TReference *result, void *screen, const char *label, unsigned int id, int iconX, int iconY);
    void ControlsAppendReference(void *vector, const RuntimeControls::TReference *reference);
    void ControlsReleaseReference(RuntimeControls::TReference *reference);
    void ControlsAddWorldRows(void *screen);
    void *ControlsGetOptions(void *client);
    void ControlsReadPairs(RuntimeControls::TPairs *pairs, void *options);
    int ControlsWritePairs(void *options, RuntimeControls::TPairs *pairs);
    void ControlsAppendStringPair(RuntimeControls::TPairs *pairs, const RuntimeControls::TString *key, const RuntimeControls::TString *value);
    int ControlsOriginalCategory(void *screen, unsigned int button);
    int ControlsOriginalRelease(void *screen, int action);
    void ControlsOriginalDestructor(void *screen);
    void *TouchGetCurrentScreen(void *client);
    void TouchRenderBottomHud(void *hud, int x, int y, float partialTick);
    void TouchSetGuiColor(void *state, const float *color);
    void TouchBegin(void *tessellator, int vertices);
    void TouchDraw(void *tessellator, void *material);
    extern unsigned char touchTessellator[];
    extern unsigned char touchGuiMaterial[];
    extern float touchGuiColorState[];
    extern float touchWhiteColor[];
    void ControlsCreateThread(RuntimeControls::TState *state, void *stackTop);
    void ControlsJoinThread(unsigned int handle);
    void ControlsSleep();
    int ControlsAccessoryStatus();
    int ControlsAccessoryStreaming();
    int ControlsAccessoryStart(int unused, int period);
    void ControlsAccessoryStop();
    extern unsigned char controlsAccessoryInitialized;
}

namespace RuntimeControls {
    constexpr unsigned int firstButtonId = 0x1000;
    constexpr unsigned int rowCount = 21;
    constexpr unsigned int resetRow = 17;
    constexpr unsigned int cancelRow = 18;
    constexpr unsigned int clearRow = 19;
    constexpr unsigned int statusRow = 20;
    constexpr const char *saveKey = "mc3ds_controls_v1";
}

RuntimeControls::TState *RuntimeControls::GetState() {
    if (runtimeControls == nullptr) {
        auto *state = static_cast<TState *>(ControlsAllocate(sizeof(TState)));
        if (state == nullptr) {
            return nullptr;
        }

        auto *bytes = reinterpret_cast<volatile unsigned char *>(state);
        for (auto index = 0U; index < sizeof(TState); ++index) {
            bytes[index] = 0;
        }

        ControlSettings::Initialize(state->config);
        state->captureIndex = -1;
        runtimeControls = state;
    }

    return runtimeControls;
}

char *RuntimeControls::Append(char *text, const char *value) {
    while (*value != 0) {
        *text++ = *value++;
    }

    *text = 0;
    return text;
}

bool RuntimeControls::Equals(const char *left, const char *right) {
    while (*left != 0 && *left == *right) {
        ++left;
        ++right;
    }

    return *left == *right;
}

bool RuntimeControls::IsControlsPage(const TState &state) {
    if (state.screen == nullptr || *reinterpret_cast<const unsigned int *>(state.screen + 284) != 2) {
        return false;
    }

    return TouchGetCurrentScreen(*reinterpret_cast<void **>(state.screen + 4)) == state.screen;
}

void RuntimeControls::RefreshLabels(TState &state) {
    if (state.screen == nullptr) {
        return;
    }

    char label[160];
    const auto &profile = state.config.profiles[static_cast<unsigned int>(state.config.mode)];
    for (auto row = 0U; row < rowCount; ++row) {
        auto *text = label;
        if (row == 0) {
            text = Append(text, "Mode: ");
            Append(text, ControlSettings::ModeName(state.config.mode));
        } else if (row == 1) {
            Append(text, state.editMenus ? "Bindings: menus" : "Bindings: gameplay");
        } else if (row == resetRow) {
            Append(text, "Reset this mode to defaults");
        } else if (row == cancelRow) {
            Append(text, state.captureIndex >= 0 ? "Touch here to cancel binding" : "Select an action to rebind");
        } else if (row == clearRow) {
            Append(text, state.captureIndex >= 0 ? "Touch here to unbind" : "Settings use stock navigation");
        } else if (row == statusRow) {
            Append(text, state.status[0] != 0 ? state.status : "Hold buttons, then release");
        } else {
            const auto index = row - 2 + (state.editMenus ? ControlSettings::gameplayBindingCount : 0);
            if (index >= ControlSettings::bindingCount) {
                Append(text, "");
            } else {
                if (state.captureIndex == static_cast<int>(index)) {
                    Append(text, "Press keys; tap again to unbind");
                } else {
                    text = Append(text, ControlSettings::ActionName(index));
                    text = Append(text, ": ");
                    ControlSettings::DescribeBinding(profile.bindings[index], text);
                }
            }
        }

        if (state.buttons[row] != nullptr) {
            ControlsStringAssign(reinterpret_cast<TString *>(state.buttons[row] + 108), label);
            ControlsUpdateButtonLabel(state.buttons[row]);
        }
    }
}

void RuntimeControls::Changed(TState &state) {
    if (state.options != nullptr) {
        state.options[456] = 1;
    }

    for (auto &input : state.inputs) {
        input.suppressed |= input.previousButtons;
    }

    state.touch.input.canceled = true;
    state.workerMode = state.config.mode;
    RefreshLabels(state);
}

void RuntimeControls::BuildControlRows(unsigned char *screen) {
    ControlsAddWorldRows(screen);
    auto *state = GetState();
    if (state == nullptr) {
        return;
    }

    state->screen = screen;
    state->options = static_cast<unsigned char *>(ControlsGetOptions(*reinterpret_cast<void **>(screen + 4)));
    state->captureIndex = -1;
    state->status[0] = 0;
    for (auto index = 0U; index < rowCount; ++index) {
        TReference reference;
        ControlsCreateButton(&reference, screen, "Controls", firstButtonId + index, 240, 144);
        const auto *children = *reinterpret_cast<TReference **>(reference.object + 56);
        state->buttons[index] = children[0].object;
        ControlsAppendReference(screen + 344, &reference);
        ControlsReleaseReference(&reference);
    }

    RefreshLabels(*state);
}

int RuntimeControls::HandleControlButton(unsigned char *screen, unsigned int button) {
    auto *state = GetState();
    if (state == nullptr || screen != state->screen || button < firstButtonId || button >= firstButtonId + rowCount) {
        return ControlsOriginalCategory(screen, button);
    }

    const auto row = button - firstButtonId;
    const auto selectedIndex = row - 2 + (state->editMenus ? ControlSettings::gameplayBindingCount : 0);
    if (state->captureIndex >= 0 && (row == clearRow || (row >= 2 && row < resetRow && selectedIndex == static_cast<unsigned int>(state->captureIndex)))) {
        ControlSettings::SetBinding(state->config, static_cast<unsigned int>(state->captureIndex), 0);
        state->captureIndex = -1;
        Append(state->status, "Binding cleared");
    } else if (state->captureIndex >= 0) {
        state->captureIndex = -1;
        Append(state->status, "Binding canceled");
    } else if (row == 0) {
        const auto nextMode = static_cast<unsigned int>(state->config.mode) + 1;
        state->config.mode = static_cast<ControlSettings::EMode>(nextMode == ControlSettings::modeCount ? 0 : nextMode);
        state->status[0] = 0;
    } else if (row == 1) {
        state->editMenus = !state->editMenus;
        state->status[0] = 0;
    } else if (row == resetRow) {
        ControlSettings::ResetProfile(state->config.profiles[static_cast<unsigned int>(state->config.mode)], state->config.mode);
        Append(state->status, "Defaults restored");
    } else if (row >= 2 && row < resetRow) {
        const auto index = row - 2 + (state->editMenus ? ControlSettings::gameplayBindingCount : 0);
        if (index < ControlSettings::bindingCount) {
            state->captureIndex = static_cast<int>(index);
            state->captureButtons = 0;
            state->captureReady = false;
            state->status[0] = 0;
        }
    }

    Changed(*state);
    return 1;
}

int RuntimeControls::DispatchControlRelease(unsigned char *screen, int action) {
    auto *state = runtimeControls;
    if (state != nullptr && state->screen == screen && *reinterpret_cast<unsigned int *>(screen + 284) == 2) {
        for (auto row = 0U; row < rowCount; ++row) {
            if (state->buttons[row] != nullptr && state->buttons[row][48] != 0) {
                return HandleControlButton(screen, firstButtonId + row);
            }
        }
    }

    return ControlsOriginalRelease(screen, action);
}

void RuntimeControls::DestroyControlRows(unsigned char *screen) {
    if (runtimeControls != nullptr && runtimeControls->screen == screen) {
        runtimeControls->screen = nullptr;
        runtimeControls->captureIndex = -1;
    }

    ControlsOriginalDestructor(screen);
}

void RuntimeControls::Capture(TState &state, unsigned int buttons, bool touching) {
    if (!state.captureReady) {
        state.captureReady = buttons == 0 && !touching;
        return;
    }

    state.captureButtons |= buttons & ControlSettings::bindableButtons;
    if (buttons == 0 && state.captureButtons != 0) {
        const auto changed = ControlSettings::SetBinding(state.config, static_cast<unsigned int>(state.captureIndex), state.captureButtons);
        Append(state.status, changed ? "Binding saved" : "Invalid binding");
        state.captureIndex = -1;
        Changed(state);
    }
}

void RuntimeControls::UpdateRuntimeControls(unsigned char *inputManager, unsigned int controller) {
    auto *state = GetState();
    if (state == nullptr || controller >= 4) {
        return;
    }

    auto &sample = *reinterpret_cast<ControlSettings::TSample *>(inputManager + 352);
    const auto physical = sample.buttons;
    const auto gameplay = TouchStick::IsGameplay(state->touch.hud);
    const auto controlsPage = IsControlsPage(*state);
    if (controller == 3) {
        state->rawButtons = physical;
    }

    if (controlsPage) {
        //Stock navigation still tracks held buttons so leaving this page cannot synthesize a press.
        state->inputs[controller].previousButtons = physical;
        state->inputs[controller].suppressed = physical;
        state->inputs[controller].chordConsumed = 0;
    }

    const auto capturing = controlsPage && state->captureIndex >= 0;
    if (capturing) {
        if (controller == 3) {
            Capture(*state, physical, inputManager[1164] != 0);
        }

        sample = {};
    } else if (!controlsPage) {
        ControlSettings::Apply(state->config, state->inputs[controller], sample,
            gameplay ? ControlSettings::EContext::GAMEPLAY : ControlSettings::EContext::MENU);
    }

    const auto *coordinates = reinterpret_cast<const short *>(inputManager + 1160);
    const auto touchMode = state->config.mode == ControlSettings::EMode::TOUCH_STICK;
    const auto consumed = TouchStick::Update(state->touch.input, coordinates[0], coordinates[1], inputManager[1164] != 0,
        gameplay && touchMode);
    if (controller == 3 && touchMode && gameplay) {
        sample.lookX = state->touch.input.lookX;
        sample.lookY = state->touch.input.lookY;
    }

    if (consumed) {
        inputManager[1164] = 0;
        inputManager[1172] = 0;
    }
}

void RuntimeControls::DrawRuntimeControls(void *hud, int x, int y, float partialTick) {
    TouchRenderBottomHud(hud, x, y, partialTick);
    auto *state = GetState();
    if (state == nullptr) {
        return;
    }

    state->touch.hud = hud;
    if (state->config.mode != ControlSettings::EMode::TOUCH_STICK || !TouchStick::IsGameplay(hud)) {
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
    TouchStick::DrawDisk(TouchStick::centerX, TouchStick::centerY, TouchStick::radius, rim);
    TouchStick::DrawDisk(TouchStick::centerX, TouchStick::centerY, TouchStick::radius - 2, inside);
    TouchStick::DrawDisk(TouchStick::centerX + state->touch.input.lookX * 0.4f,
        TouchStick::centerY - state->touch.input.lookY * 0.4f, 10.0f, knob);
    TouchDraw(touchTessellator, touchGuiMaterial);
    TouchSetGuiColor(touchGuiColorState, previousColor);
}

void RuntimeControls::ForgetRuntimeHud(void *hud) {
    if (runtimeControls != nullptr && runtimeControls->touch.hud == hud) {
        runtimeControls->touch.hud = nullptr;
    }
}

void RuntimeControls::LoadControlSettings(TPairs *pairs, unsigned char *options) {
    ControlsReadPairs(pairs, options);
    auto *state = GetState();
    if (state == nullptr) {
        return;
    }

    state->options = options;
    for (auto *pair = pairs->begin; pair != pairs->end;) {
        if (!Equals(pair->key.data, saveKey)) {
            ++pair;
            continue;
        }

        const auto size = *(reinterpret_cast<const unsigned int *>(pair->value.data) - 1);
        ControlSettings::Decode(state->config, pair->value.data, size);
        state->workerMode = state->config.mode;
        ControlsStringRelease(&pair->key);
        ControlsStringRelease(&pair->value);
        --pairs->end;
        for (auto *next = pair; next != pairs->end; ++next) {
            *next = next[1];
        }
    }
}

int RuntimeControls::SaveControlSettings(unsigned char *options, TPairs *pairs) {
    auto *state = GetState();
    if (state != nullptr) {
        char value[ControlSettings::encodedSize + 1];
        ControlSettings::Encode(state->config, value);
        TString key;
        TString text;
        unsigned int temporary;
        ControlsStringConstruct(&key, saveKey, &temporary);
        ControlsStringConstruct(&text, value, &temporary);
        ControlsAppendStringPair(pairs, &key, &text);
        ControlsStringRelease(&text);
        ControlsStringRelease(&key);
    }

    return ControlsWritePairs(options, pairs);
}

void RuntimeControls::StartControlAccessory() {
    auto *state = GetState();
    if (state == nullptr || state->workerStarted != 0 || state->config.mode != ControlSettings::EMode::CIRCLE_PAD_PRO) {
        return;
    }

    state->workerStarted = 1;
    const auto stackTop = reinterpret_cast<unsigned int>(state->workerStack + sizeof(state->workerStack)) & ~7U;
    ControlsCreateThread(state, reinterpret_cast<void *>(stackTop));
}

void RuntimeControls::ControlAccessoryWorker(TState *state) {
    while (state->stopWorker == 0) {
        if (controlsAccessoryInitialized != 0) {
            if (state->workerMode == ControlSettings::EMode::CIRCLE_PAD_PRO) {
                if (ControlsAccessoryStatus() == 2) {
                    ControlsAccessoryStop();
                }

                if (ControlsAccessoryStreaming() == 0) {
                    state->workerResult = ControlsAccessoryStart(0, 8);
                    for (auto retry = 0; retry < 25 && state->stopWorker == 0; ++retry) {
                        ControlsSleep();
                    }
                }
            } else if (ControlsAccessoryStreaming() != 0) {
                ControlsAccessoryStop();
            }
        }

        ControlsSleep();
    }
}

void RuntimeControls::StopControlAccessory() {
    if (runtimeControls != nullptr) {
        runtimeControls->stopWorker = 1;
        if (runtimeControls->threadHandle != 0) {
            ControlsJoinThread(runtimeControls->threadHandle);
        }
    }
}
