#include "RuntimeControls.h"
#ifdef MC3DS_NATIVE_MENU
#include "NativeMenus.h"
#include "RuntimeWorld.h"
#endif

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
void ControlsSetFloat(void *options, const void *key, float value);
void ControlsSetBoolean(void *options, const void *key, unsigned int value);
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
    constexpr const char *pageNames[] = {"Gameplay binds", "Menu binds", "Camera", "Touch stick", "Display", "Binding tools", "Overlay"};
    using ESetting = ControlSettings::ESetting;
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
        state->copySource = ControlSettings::EMode::CIRCLE_PAD_PRO;
        __atomic_store_n(&runtimeControls, state, __ATOMIC_RELEASE);
    }

    return runtimeControls;
}

char *RuntimeControls::AppendNumber(char *text, unsigned int value) {
    char digits[10];
    auto count = 0U;
    do {
        digits[count++] = static_cast<char>('0' + value % 10);
        value /= 10;
    } while (value != 0);

    while (count != 0) {
        *text++ = digits[--count];
    }

    *text = 0;
    return text;
}

void RuntimeControls::DescribeSetting(const TState &state, ESetting setting, char *text) {
    text = Append(Append(text, ControlSettings::SettingName(setting)), ": ");
    const auto value = ControlSettings::GetSetting(state.config, setting);
    if (setting == ESetting::OVERLAY) {
        const char *names[] = {"Off", "FPS", "Full"};
        Append(text, names[value]);
    } else if (setting == ESetting::PARTICLES) {
        const char *names[] = {"Normal", "Reduced", "Hidden"};
        Append(text, names[value]);
    } else if (setting == ESetting::MAP_RATE) {
        const char *names[] = {"Normal", "4 Hz", "1 Hz", "Paused"};
        Append(text, names[value]);
    } else if (setting == ESetting::MODIFIER_TOGGLE) {
        Append(text, value != 0 ? "Toggle" : "Hold");
    } else if (setting == ESetting::WEATHER) {
        Append(text, value != 0 ? "Hidden" : "Normal");
    } else if (setting == ESetting::LOW_MEMORY_TEXTURES) {
        Append(text, value != 0 ? "On*" : "Off*");
    } else if (setting == ESetting::TOUCH_HIDE_IDLE || setting == ESetting::CONFLICT_HINTS) {
        Append(text, value != 0 ? "On" : "Off");
    } else if (setting == ESetting::CAMERA_CURVE) {
        const char *names[] = {"Linear", "Precision", "Quick"};
        Append(text, names[value]);
    } else if (setting == ESetting::OVERLAY_POSITION) {
        const char *names[] = {"Top left", "Top right", "Bottom left", "Bottom right"};
        Append(text, names[value]);
    } else if (setting == ESetting::FRAME_GRAPH) {
        Append(text, value != 0 ? "On" : "Off");
    } else {
        text = AppendNumber(text, value);
        if (setting == ESetting::LOOK_X || setting == ESetting::LOOK_Y || setting == ESetting::TOUCH_OPACITY) {
            Append(text, "%");
        }
    }
}

void RuntimeControls::DescribePageRow(const TState &state, unsigned int row, char *text) {
    *text = 0;
    if (state.page == EPage::CAMERA) {
        if (row == 2 && state.options != nullptr) {
            text = AppendNumber(Append(text, "Native sensitivity: "),
                static_cast<unsigned int>(*reinterpret_cast<const float *>(state.options + 88) * 100.0f + 0.5f));
            Append(text, "%");
        } else if ((row == 3 || row == 4) && state.options != nullptr) {
            text = Append(text, row == 3 ? "Invert Y: " : "Auto-jump: ");
            Append(text, state.options[row == 3 ? 96 : 116] != 0 ? "On" : "Off");
        } else if (row >= 5 && row <= 7) {
            DescribeSetting(state, static_cast<ESetting>(row - 5), text);
        } else if (row == 8) {
            Append(text, "Reset camera tuning");
        } else if (row == 13) {
            Append(text, "Look binds apply in ABXY mode");
        } else if (row == 14 || row == 15) {
            DescribeSetting(state, row == 14 ? ESetting::CAMERA_CURVE : ESetting::CAMERA_ACCELERATION, text);
        }
    } else if (state.page == EPage::TOUCH && row >= 2 && row <= 7) {
        DescribeSetting(state, static_cast<ESetting>(static_cast<unsigned int>(ESetting::TOUCH_X) + row - 2), text);
    } else if (state.page == EPage::DISPLAY) {
        if (row >= 2 && row <= 6) {
            DescribeSetting(state, static_cast<ESetting>(static_cast<unsigned int>(ESetting::OVERLAY) + row - 2), text);
        } else if (row == 7) {
            Append(text, "*Textures: restart game to apply");
        } else if (row == 8) {
            Append(text, "Visual reductions keep world ticks");
        } else if (row == 9) {
            const char *names[] = {"Custom", "Quality", "Balanced", "Low memory"};
            Append(Append(text, "Preset: "), names[ControlSettings::PerformancePreset(state.config)]);
#ifdef MC3DS_NATIVE_MENU
        } else if (row >= 10 && row <= 13) {
            DescribeSetting(state, static_cast<ESetting>(static_cast<unsigned int>(ESetting::RENDER_DISTANCE) + row - 10), text);
        } else if (row == 14) {
            Append(text, "World settings need a full restart");
        } else if (row == 15) {
            Append(text, "Render is capped by nearby radius");
#endif
        }
    } else if (state.page == EPage::OVERLAY && row >= 2 && row <= 5) {
        DescribeSetting(state, row == 5 ? ESetting::FRAME_GRAPH : static_cast<ESetting>(static_cast<unsigned int>(ESetting::OVERLAY_POSITION) + row - 2), text);
    } else if (state.page == EPage::TOOLS) {
        if (row == 2) {
            Append(Append(text, "Selected: "), ControlSettings::ActionName(state.selectedBinding));
        } else if (row == 3) {
            Append(text, "Reset selected binding");
        } else if (row == 4) {
            Append(text, "Unbind selected action");
        } else if (row == 5) {
            Append(Append(text, "Copy from: "), ControlSettings::ModeName(state.copySource));
        } else if (row == 6) {
            Append(text, "Copy source bindings to this mode");
        } else if (row == 7) {
            DescribeSetting(state, ESetting::CONFLICT_HINTS, text);
        } else if (row == 8) {
            Append(text, "* = shared; duplicates are allowed");
        }
    }
}

void RuntimeControls::ActivatePageRow(TState &state, unsigned int row) {
    if (state.page == EPage::CAMERA) {
        if (row == 2 && state.options != nullptr) {
            const auto current = *reinterpret_cast<const float *>(state.options + 88);
            const auto next = current + 0.05f;
            ControlsSetFloat(state.options, reinterpret_cast<const void *>(0xa31c64), next > 1.001f ? 0.0f : next);
        } else if ((row == 3 || row == 4) && state.options != nullptr) {
            ControlsSetBoolean(state.options, reinterpret_cast<const void *>(row == 3 ? 0xa31c5c : 0xa31d14),
                state.options[row == 3 ? 96 : 116] == 0);
        } else if (row >= 5 && row <= 7) {
            ControlSettings::CycleSetting(state.config, static_cast<ESetting>(row - 5));
        } else if (row == 8) {
            ControlSettings::SetSetting(state.config, ESetting::LOOK_X, 100);
            ControlSettings::SetSetting(state.config, ESetting::LOOK_Y, 100);
            ControlSettings::SetSetting(state.config, ESetting::MODIFIER_TOGGLE, 0);
            ControlSettings::SetSetting(state.config, ESetting::CAMERA_CURVE, 0);
            ControlSettings::SetSetting(state.config, ESetting::CAMERA_ACCELERATION, 0);
            if (state.options != nullptr) {
                ControlsSetFloat(state.options, reinterpret_cast<const void *>(0xa31c64), 0.33f);
                ControlsSetBoolean(state.options, reinterpret_cast<const void *>(0xa31c5c), 0);
            }
        } else if (row == 14 || row == 15) {
            ControlSettings::CycleSetting(state.config, row == 14 ? ESetting::CAMERA_CURVE : ESetting::CAMERA_ACCELERATION);
        }
    } else if (state.page == EPage::TOUCH && row >= 2 && row <= 7) {
        ControlSettings::CycleSetting(state.config, static_cast<ESetting>(static_cast<unsigned int>(ESetting::TOUCH_X) + row - 2));
    } else if (state.page == EPage::DISPLAY && row >= 2 && row <= 6) {
        ControlSettings::CycleSetting(state.config, static_cast<ESetting>(static_cast<unsigned int>(ESetting::OVERLAY) + row - 2));
        if (row == 6) {
            Append(state.status, "Texture change needs a full restart");
        }
    } else if (state.page == EPage::DISPLAY && row == 9) {
        ControlSettings::ApplyPerformancePreset(state.config, ControlSettings::PerformancePreset(state.config) % 3 + 1);
        Append(state.status, "Texture changes need a full restart");
#ifdef MC3DS_NATIVE_MENU
    } else if (state.page == EPage::DISPLAY && row >= 10 && row <= 13) {
        ControlSettings::CycleSetting(state.config, static_cast<ESetting>(static_cast<unsigned int>(ESetting::RENDER_DISTANCE) + row - 10));
#endif
    } else if (state.page == EPage::OVERLAY && row >= 2 && row <= 5) {
        ControlSettings::CycleSetting(state.config, row == 5 ? ESetting::FRAME_GRAPH : static_cast<ESetting>(static_cast<unsigned int>(ESetting::OVERLAY_POSITION) + row - 2));
    } else if (state.page == EPage::TOOLS) {
        if (row == 2) {
            state.selectedBinding = (state.selectedBinding + 1) % ControlSettings::bindingCount;
        } else if (row == 3) {
            ControlSettings::ResetBinding(state.config, state.selectedBinding);
            Append(state.status, "Selected binding restored");
        } else if (row == 4) {
            ControlSettings::SetBinding(state.config, state.selectedBinding, 0);
            Append(state.status, "Selected action unbound");
        } else if (row == 5) {
            state.copySource = static_cast<ControlSettings::EMode>((static_cast<unsigned int>(state.copySource) + 1) % ControlSettings::modeCount);
        } else if (row == 6) {
            ControlSettings::CopyBindings(state.config, state.copySource);
            Append(state.status, "Bindings copied; camera unchanged");
        } else if (row == 7) {
            ControlSettings::CycleSetting(state.config, ESetting::CONFLICT_HINTS);
        }
    }
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

unsigned int RuntimeControls::RowBinding(const TState &state, unsigned int row) {
    if (row >= 2 && row < resetRow) {
        if (state.page == EPage::GAMEPLAY) {
            return row - 2;
        }

        if (state.page == EPage::MENUS && row - 2 < ControlSettings::menuBindingCount) {
            return row - 2 + ControlSettings::gameplayBindingCount;
        }

        if (state.page == EPage::CAMERA && row >= 9 && row <= 12) {
            return row - 9 + ControlSettings::cameraBindingBegin;
        }
    }

    return ControlSettings::bindingCount;
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
            Append(Append(text, "Page: "), pageNames[static_cast<unsigned int>(state.page)]);
        } else if (row == resetRow) {
            Append(text, "Reset this mode to defaults");
        } else if (row == cancelRow) {
            Append(text, state.captureIndex >= 0 ? "Touch here to cancel binding" : "Select an action to rebind");
        } else if (row == clearRow) {
            Append(text, state.captureIndex >= 0 ? "Touch here to unbind" : "Settings use stock navigation");
        } else if (row == statusRow) {
            Append(text, state.status[0] != 0 ? state.status : "Hold buttons, then release");
        } else {
            const auto index = RowBinding(state, row);
            if (index < ControlSettings::bindingCount) {
                if (state.captureIndex == static_cast<int>(index)) {
                    Append(text, "Press keys; tap again to unbind");
                } else {
                    text = Append(text, ControlSettings::ActionName(index));
                    if (ControlSettings::GetSetting(state.config, ESetting::CONFLICT_HINTS) != 0 && ControlSettings::IsShared(state.config, index)) {
                        text = Append(text, " *");
                    }

                    text = Append(text, ": ");
                    ControlSettings::DescribeBinding(profile.bindings[index], text);
                }
            } else {
                DescribePageRow(state, row, text);
            }
        }

#ifdef MC3DS_NATIVE_MENU
        if (NativeMenus::UpdateRow(state, row, label)) {
            continue;
        }
#endif
        if (state.buttons[row] != nullptr) {
            auto *current = reinterpret_cast<TString *>(state.buttons[row] + 108);
            if (!Equals(current->data, label)) {
                ControlsStringAssign(current, label);
                ControlsUpdateButtonLabel(state.buttons[row]);
            }
        }
    }
}

void RuntimeControls::Changed(TState &state) {
    if (state.options != nullptr) {
        state.options[456] = 1;
    }

    for (auto &input : state.inputs) {
        input.suppressed |= input.previousButtons;
        input.cameraLatched = false;
    }

    state.touch.input.canceled = true;
    state.mapRate = 0xffffffffU;
    state.workerMode = state.config.mode;
    RefreshLabels(state);
}

void RuntimeControls::BuildControlRows(unsigned char *screen) {
#ifdef MC3DS_NATIVE_MENU
    if (screen[280] == 2) {
        return;
    }
#endif
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
    const auto selectedIndex = RowBinding(*state, row);
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
        state->page = static_cast<EPage>((static_cast<unsigned int>(state->page) + 1) % static_cast<unsigned int>(EPage::COUNT));
        state->status[0] = 0;
    } else if (row == resetRow) {
        ControlSettings::ResetMode(state->config);
        Append(state->status, "Defaults restored");
    } else if (selectedIndex < ControlSettings::bindingCount) {
        state->captureIndex = static_cast<int>(selectedIndex);
        state->selectedBinding = selectedIndex;
        state->captureButtons = 0;
        state->captureReady = false;
        state->status[0] = 0;
    } else if (state->page >= EPage::CAMERA) {
        ActivatePageRow(*state, row);
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
#ifdef MC3DS_NATIVE_MENU
    NativeMenus::ForgetScreen(screen);
#endif
    if (runtimeControls != nullptr && runtimeControls->screen == screen) {
        runtimeControls->screen = nullptr;
        runtimeControls->captureIndex = -1;
    }

    ControlsOriginalDestructor(screen);
}

void RuntimeControls::Capture(TState &state, unsigned int buttons, bool touching) {
    if (!state.captureReady) {
        state.captureReady = buttons == 0 && !touching;
        if (state.captureReady) {
            Append(state.status, "Press a button or chord, then release");
            RefreshLabels(state);
        }
        return;
    }

    const auto previous = state.captureButtons;
    state.captureButtons |= buttons & ControlSettings::bindableButtons;
    if (previous != state.captureButtons) {
        ControlSettings::DescribeBinding(state.captureButtons, Append(state.status, "Release to save: "));
        RefreshLabels(state);
    }
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
#ifdef MC3DS_NATIVE_MENU
        NativeMenus::Poll(*state);
#endif
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
    const auto consumed = TouchStick::UpdateConfigured(state->touch.input, coordinates[0], coordinates[1], inputManager[1164] != 0,
        gameplay && touchMode, GetTouchSettings(*state));
    if (controller == 3 && touchMode && gameplay) {
        sample.lookX = state->touch.input.lookX;
        sample.lookY = state->touch.input.lookY;
        ControlSettings::ScaleLook(state->config, sample);
    }

    if (consumed) {
        inputManager[1164] = 0;
        inputManager[1172] = 0;
    }
}

TouchStick::TSettings RuntimeControls::GetTouchSettings(const TState &state) {
    return {static_cast<int>(ControlSettings::GetSetting(state.config, ESetting::TOUCH_X)),
        static_cast<int>(ControlSettings::GetSetting(state.config, ESetting::TOUCH_Y)),
        static_cast<int>(ControlSettings::GetSetting(state.config, ESetting::TOUCH_RADIUS)),
        static_cast<int>(ControlSettings::GetSetting(state.config, ESetting::TOUCH_DEAD_ZONE))};
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

    const auto opacity = ControlSettings::GetSetting(state->config, ESetting::TOUCH_OPACITY);
    if (opacity == 0 || (ControlSettings::GetSetting(state->config, ESetting::TOUCH_HIDE_IDLE) != 0 && !state->touch.input.ownsTouch)) {
        return;
    }

    const auto settings = GetTouchSettings(*state);
    const auto alpha = opacity / 65.0f;
    const float rim[] = {0.82f, 0.88f, 0.92f, 0.65f * alpha};
    const float inside[] = {0.08f, 0.12f, 0.18f, 0.42f * alpha};
    const float knob[] = {0.9f, 0.95f, 1.0f, opacity > 76 ? 1.0f : 0.85f * alpha};
    float previousColor[4];
    for (auto index = 0; index < 4; ++index) {
        previousColor[index] = touchGuiColorState[index];
    }

    TouchSetGuiColor(touchGuiColorState, touchWhiteColor);
    TouchBegin(touchTessellator, 288);
    TouchStick::DrawDisk(settings.x, settings.y, settings.radius, rim);
    TouchStick::DrawDisk(settings.x, settings.y, settings.radius - 2, inside);
    const auto travel = (settings.radius - 12) * 0.01f;
    TouchStick::DrawDisk(settings.x + state->touch.input.lookX * travel,
        settings.y - state->touch.input.lookY * travel, 10.0f, knob);
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
        state->activeLowMemoryTextures = ControlSettings::GetSetting(state->config, ESetting::LOW_MEMORY_TEXTURES);
        ControlsStringRelease(&pair->key);
        ControlsStringRelease(&pair->value);
        --pairs->end;
        for (auto *next = pair; next != pairs->end; ++next) {
            *next = next[1];
        }
    }

#ifdef MC3DS_NATIVE_MENU
    RuntimeWorld::Apply(state->config);
#endif
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
