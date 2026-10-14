#include "NativeMenus.h"
#include "RuntimeWorld.h"
#include "DebugOverlay.h"

extern "C" {
extern RuntimeControls::TState *runtimeControls;
void *ControlsAllocate(unsigned int size);
void ControlsStringConstruct(RuntimeControls::TString *text, const char *value, void *temporary);
void ControlsStringAssign(RuntimeControls::TString *text, const char *value);
void ControlsStringRelease(RuntimeControls::TString *text);
void ControlsCreateButton(RuntimeControls::TReference *result, void *screen, const char *label, unsigned int id, int iconX, int iconY);
void ControlsAppendReference(void *vector, const RuntimeControls::TReference *reference);
void ControlsReleaseReference(RuntimeControls::TReference *reference);
void *ControlsGetOptions(void *client);
void ControlsSetFloat(void *options, const void *key, float value);
void ControlsSetBoolean(void *options, const void *key, unsigned int value);
void NativeClearReferences(void *vector);
void NativeCreateSlider(RuntimeControls::TReference *result, void *screen, const void *key);
void NativeCreateBoolean(RuntimeControls::TReference *result, void *screen, const void *key);
void NativeCreateDropdown(RuntimeControls::TReference *result, void *screen, const void *key,
    const char *const *choices, unsigned int count, unsigned int id, bool enabled);
void NativeSelectCategory(void *screen);
void NativeOriginalOptionsInitialize(void *screen);
void NativeOriginalOptionsBack(void *screen);
void NativeOriginalMainInitialize(void *screen);
void NativeOriginalPauseInitialize(void *screen);
int NativeOriginalMainButton(void *screen, void *button);
int NativeOriginalPauseButton(void *screen, void *button);
int NativeOriginalDropdownKey(void *widget, void *client, int action);
void NativeAddMenuButton(void *screen, unsigned int id, const RuntimeControls::TString *label, int x, int y);
void NativeOpenOptions(void *chooser, unsigned int mode);
void NativeDropdownSelect(void *widget, unsigned int index);
void NativeMapSelection(void *screen);
void NativeUpdateSelection(void *screen, unsigned int index);
void NativeSetScroll(void *scroll, float value);
void NativeOriginalRender(void *screen, int x, int y, unsigned int flags);
void TouchSetGuiColor(void *state, const float *color);
void TouchBegin(void *tessellator, int vertices);
void TouchDraw(void *tessellator, void *material);
extern unsigned char touchTessellator[];
extern unsigned char touchGuiMaterial[];
extern float touchGuiColorState[];
extern float touchWhiteColor[];
}

namespace NativeMenus {
    using RuntimeControls::TReference;
    using RuntimeControls::TString;
    using RuntimeControls::TState;
    using RuntimeControls::EPage;
    using ControlSettings::ESetting;

    enum class EWidget : unsigned char {
        BUTTON,
        SLIDER,
        BOOLEAN,
        DROPDOWN
    };

    struct TDescriptor {
        unsigned int unused;
        TString label;
    };

    struct TRow {
        TDescriptor descriptor;
        unsigned char *widget;
        unsigned short minimum;
        unsigned short maximum;
        unsigned short step;
        EWidget kind;
    };

    struct TMenu {
        unsigned char *screen;
        EPage page;
        TRow rows[21];
        unsigned int selections[7];
        float scrollOffsets[7];
        unsigned char *activeDropdown;
    };

    static_assert(sizeof(TMenu) == 488, "Native menu descriptor budget changed");

    constexpr unsigned int menuButtonId = 0x2000;
    constexpr unsigned int firstButtonId = 0x1000;
    constexpr const char *pageNames[] = {"Gameplay binds", "Menu binds", "Camera", "Touch stick", "Display", "Binding tools", "Overlay"};
    constexpr const char *modeNames[] = {"L + Circle Pad", "Circle Pad Pro", "Touch stick", "ABXY camera"};
    constexpr const char *overlayNames[] = {"Off", "FPS", "Full"};
    constexpr const char *particleNames[] = {"Normal", "Reduced", "Hidden"};
    constexpr const char *mapNames[] = {"Normal", "4 Hz", "1 Hz", "Paused"};
    constexpr const char *cornerNames[] = {"Top left", "Top right", "Bottom left", "Bottom right"};
    constexpr const char *curveNames[] = {"Linear", "Precision", "Quick"};
    constexpr const char *presetNames[] = {"Custom", "Quality", "Balanced", "Low memory"};
    constexpr const char *countNames[] = {"1", "2", "3", "4"};
    constexpr const char *chunkNames[] = {"1 (9 chunks)", "2 (25 chunks)", "3 (49 chunks)"};
    constexpr const char *radiusNames[] = {"8", "16", "24", "32"};

    unsigned int &Word(unsigned char *object, unsigned int offset);
    float &Float(unsigned char *object, unsigned int offset);
    TMenu *GetMenu(TState &state);
    int SettingForRow(EPage page, unsigned int row);
    unsigned int Value(const TState &state, unsigned int row);
    void SetValue(TState &state, unsigned int row, unsigned int value);
    void ConfigureRow(TRow &item, EPage page, unsigned int row);
    void BuildRows(TState &state, TMenu &menu);
    void AddMenuEntry(unsigned char *screen, bool mainMenu);
    unsigned char *ScrollControl(unsigned char *screen);
    const char *Help(const TState &state);
}

unsigned int &NativeMenus::Word(unsigned char *object, unsigned int offset) {
    return *reinterpret_cast<unsigned int *>(object + offset);
}

float &NativeMenus::Float(unsigned char *object, unsigned int offset) {
    return *reinterpret_cast<float *>(object + offset);
}

NativeMenus::TMenu *NativeMenus::GetMenu(TState &state) {
    if (state.nativeMenu == nullptr) {
        auto *menu = static_cast<TMenu *>(ControlsAllocate(sizeof(TMenu)));
        if (menu == nullptr) {
            return nullptr;
        }

        menu->screen = nullptr;
        menu->activeDropdown = nullptr;
        for (auto &selection : menu->selections) {
            selection = 0;
        }

        for (auto &offset : menu->scrollOffsets) {
            offset = 0.0f;
        }
        for (auto &row : menu->rows) {
            row.descriptor.unused = 0;
            row.widget = nullptr;
            unsigned int temporary[2];
            ControlsStringConstruct(&row.descriptor.label, "", temporary);
        }

        state.nativeMenu = menu;
    }

    return static_cast<TMenu *>(state.nativeMenu);
}

int NativeMenus::SettingForRow(EPage page, unsigned int row) {
    if (page == EPage::DISPLAY && row >= 10 && row <= 13) {
        return static_cast<int>(ESetting::RENDER_DISTANCE) + row - 10;
    }

    if (page == EPage::CAMERA && (row == 14 || row == 15)) {
        return static_cast<int>(row == 14 ? ESetting::CAMERA_CURVE : ESetting::CAMERA_ACCELERATION);
    }

    if (page == EPage::OVERLAY && row >= 2 && row <= 5) {
        return row == 5 ? static_cast<int>(ESetting::FRAME_GRAPH) : static_cast<int>(ESetting::OVERLAY_POSITION) + row - 2;
    }

    if (page == EPage::CAMERA && row >= 5 && row <= 7) {
        return row - 5;
    }

    if (page == EPage::TOUCH && row >= 2 && row <= 7) {
        return static_cast<unsigned int>(ESetting::TOUCH_X) + row - 2;
    }

    if (page == EPage::DISPLAY && row >= 2 && row <= 6) {
        return static_cast<unsigned int>(ESetting::OVERLAY) + row - 2;
    }

    return page == EPage::TOOLS && row == 7 ? static_cast<int>(ESetting::CONFLICT_HINTS) : -1;
}

unsigned int NativeMenus::Value(const TState &state, unsigned int row) {
    if (state.page == EPage::DISPLAY && row == 9) {
        return ControlSettings::PerformancePreset(state.config);
    }

    if (row == 0) {
        return static_cast<unsigned int>(state.config.mode);
    }

    if (row == 1) {
        return static_cast<unsigned int>(state.page);
    }

    const auto setting = SettingForRow(state.page, row);
    if (setting >= 0) {
        const auto value = ControlSettings::GetSetting(state.config, static_cast<ESetting>(setting));
        return setting >= static_cast<int>(ESetting::RENDER_DISTANCE) ? (row == 12 ? value / 8 : value) - 1 : value;
    }

    if (state.page == EPage::CAMERA) {
        if (row == 2) {
            return static_cast<unsigned int>(Float(state.options, 88) * 100.0f + 0.5f);
        }

        return state.options[row == 3 ? 96 : 116] != 0;
    }

    return static_cast<unsigned int>(state.copySource);
}

void NativeMenus::SetValue(TState &state, unsigned int row, unsigned int value) {
    if (row == 0) {
        state.config.mode = static_cast<ControlSettings::EMode>(value);
    } else if (row == 1) {
        state.page = static_cast<EPage>(value);
    } else if (const auto setting = SettingForRow(state.page, row); setting >= 0) {
        if (setting >= static_cast<int>(ESetting::RENDER_DISTANCE)) {
            value = (value + 1) * (row == 12 ? 8 : 1);
        }
        ControlSettings::SetSetting(state.config, static_cast<ESetting>(setting), value);
    } else if (state.page == EPage::CAMERA && row == 2) {
        ControlsSetFloat(state.options, reinterpret_cast<const void *>(0xa31c64), value * 0.01f);
    } else if (state.page == EPage::CAMERA && (row == 3 || row == 4)) {
        ControlsSetBoolean(state.options, reinterpret_cast<const void *>(row == 3 ? 0xa31c5c : 0xa31d14), value);
    } else if (state.page == EPage::TOOLS && row == 5) {
        state.copySource = static_cast<ControlSettings::EMode>(value);
    } else if (state.page == EPage::DISPLAY && row == 9) {
        ControlSettings::ApplyPerformancePreset(state.config, value);
    }

    if (state.page == EPage::DISPLAY && row == 6) {
        RuntimeControls::Append(state.status, "Texture change needs a full restart");
    }
}

void NativeMenus::ConfigureRow(TRow &item, EPage page, unsigned int row) {
    item.widget = nullptr;
    item.kind = EWidget::BUTTON;
    item.minimum = 0;
    item.maximum = 1;
    item.step = 1;
    if (row < 2 || (page == EPage::TOOLS && row == 5)) {
        item.kind = EWidget::DROPDOWN;
        item.maximum = row == 1 ? 6 : 3;
    } else if (page == EPage::DISPLAY && row >= 10 && row <= 13) {
        item.kind = EWidget::DROPDOWN;
        item.maximum = row >= 12 ? 3 : 2;
    } else if (page == EPage::DISPLAY && row == 9) {
        item.kind = EWidget::DROPDOWN;
        item.maximum = 3;
    } else if (page == EPage::CAMERA && row == 14) {
        item.kind = EWidget::DROPDOWN;
        item.maximum = 2;
    } else if ((page == EPage::CAMERA && row == 15) || (page == EPage::OVERLAY && row >= 2 && row <= 5)) {
        item.kind = row == 2 ? EWidget::DROPDOWN : row == 5 ? EWidget::BOOLEAN : EWidget::SLIDER;
        item.minimum = row == 3 ? 50 : 0;
        item.maximum = row == 2 ? 3 : row == 3 ? 125 : row == 5 ? 1 : 100;
        item.step = row == 3 ? 5 : 1;
    } else if (page == EPage::CAMERA && row >= 2 && row <= 7) {
        item.kind = row == 3 || row == 4 || row == 7 ? EWidget::BOOLEAN : EWidget::SLIDER;
        if (item.kind == EWidget::SLIDER) {
            item.minimum = row == 2 ? 0 : 25;
            item.maximum = row == 2 ? 100 : 200;
            item.step = row == 2 ? 1 : 5;
        }
    } else if (page == EPage::TOUCH && row >= 2 && row <= 7) {
        constexpr unsigned short limits[][3] = {{72, 248, 1}, {72, 168, 1}, {28, 68, 1}, {0, 15, 1}, {0, 100, 1}, {0, 1, 1}};
        item.minimum = limits[row - 2][0];
        item.maximum = limits[row - 2][1];
        item.step = limits[row - 2][2];
        item.kind = row == 7 ? EWidget::BOOLEAN : EWidget::SLIDER;
    } else if (page == EPage::DISPLAY && row >= 2 && row <= 6) {
        item.kind = row == 4 || row == 6 ? EWidget::BOOLEAN : EWidget::DROPDOWN;
        item.maximum = row == 5 ? 3 : 2;
    } else if (page == EPage::TOOLS && row == 7) {
        item.kind = EWidget::BOOLEAN;
    }
}

bool NativeMenus::UpdateRow(TState &state, unsigned int row, const char *label) {
    auto *menu = static_cast<TMenu *>(state.nativeMenu);
    if (menu == nullptr || menu->screen != state.screen) {
        return false;
    }

    auto &item = menu->rows[row];
    if (item.kind == EWidget::BUTTON) {
        return false;
    }

    if (!RuntimeControls::Equals(item.descriptor.label.data, label)) {
        ControlsStringAssign(&item.descriptor.label, label);
    }

    if (item.widget != nullptr) {
        const auto value = Value(state, row);
        if (item.kind == EWidget::SLIDER) {
            Float(item.widget, 80) = static_cast<float>(value - item.minimum) / (item.maximum - item.minimum);
        } else if (item.kind == EWidget::BOOLEAN) {
            item.widget[248] = value != 0;
        } else if (item.widget[202] == 0) {
            NativeDropdownSelect(item.widget, value);
        }
    }

    return true;
}

void NativeMenus::BuildRows(TState &state, TMenu &menu) {
    auto *screen = menu.screen;
    menu.activeDropdown = nullptr;
    NativeClearReferences(screen + 344);
    NativeClearReferences(screen + 308);
    //Release pane and navigation ownership before allocating replacement rows.
    Word(screen, 284) = 2;
    NativeSelectCategory(screen);
    menu.page = state.page;
    state.captureIndex = -1;
    for (auto row = 0U; row < 21; ++row) {
        auto &item = menu.rows[row];
        ConfigureRow(item, state.page, row);
        state.buttons[row] = nullptr;
        if (row >= 2 && row < 17 && RuntimeControls::RowBinding(state, row) == ControlSettings::bindingCount) {
            char label[160];
            RuntimeControls::DescribePageRow(state, row, label);
            if (label[0] == 0) {
                continue;
            }
        }

        TReference reference;
        if (item.kind == EWidget::SLIDER) {
            NativeCreateSlider(&reference, screen, reinterpret_cast<const void *>(0xa31c64));
        } else if (item.kind == EWidget::BOOLEAN) {
            NativeCreateBoolean(&reference, screen, reinterpret_cast<const void *>(0xa31c5c));
        } else if (item.kind == EWidget::DROPDOWN) {
            const auto *choices = row == 1 ? pageNames : modeNames;
            if (state.page == EPage::DISPLAY && row == 9) {
                choices = presetNames;
            } else if (state.page == EPage::DISPLAY && row >= 10 && row <= 13) {
                choices = row == 11 ? chunkNames : row == 12 ? radiusNames : countNames;
            } else if (state.page == EPage::OVERLAY && row == 2) {
                choices = cornerNames;
            } else if (state.page == EPage::CAMERA && row == 14) {
                choices = curveNames;
            } else if (state.page == EPage::DISPLAY && row >= 2) {
                choices = row == 2 ? overlayNames : row == 3 ? particleNames : mapNames;
            }

            NativeCreateDropdown(&reference, screen, reinterpret_cast<const void *>(0xa31c8c), choices,
                item.maximum + 1, firstButtonId + row, false);
            //Use base option-row behavior, with no world-setting synchronization.
            Word(reference.object, 0) = 0x99d7e0;
            Word(reference.object, 4) = 0x99d884;
        } else {
            ControlsCreateButton(&reference, screen, "Controls", firstButtonId + row, 240, 144);
        }

        const auto *children = *reinterpret_cast<const TReference **>(reference.object + 56);
        item.widget = children[0].object;
        if (item.kind == EWidget::BUTTON) {
            state.buttons[row] = item.widget;
        } else {
            Word(reference.object, 96) = reinterpret_cast<unsigned int>(&item.descriptor);
            if (item.kind == EWidget::SLIDER) {
                Word(item.widget, 108) = 0;
                Word(item.widget, 20) = 240;
            } else if (item.kind == EWidget::BOOLEAN) {
                Word(item.widget, 244) = 0;
            }
        }

        ControlsAppendReference(screen + 344, &reference);
        ControlsReleaseReference(&reference);
    }

    RuntimeControls::RefreshLabels(state);
    Word(screen, 284) = 2;
    Word(screen, 288) = 0;
    NativeSelectCategory(screen);
    const auto selection = menu.selections[static_cast<unsigned int>(state.page)];
    Word(screen, 168) = selection;
    NativeMapSelection(screen);
    NativeUpdateSelection(screen, selection);
    NativeSetScroll(ScrollControl(screen), menu.scrollOffsets[static_cast<unsigned int>(state.page)]);
}

void NativeMenus::Poll(TState &state) {
    auto *menu = static_cast<TMenu *>(state.nativeMenu);
    if (menu == nullptr || menu->screen != state.screen || !RuntimeControls::IsControlsPage(state) || state.captureIndex >= 0) {
        return;
    }

    menu->activeDropdown = nullptr;
    for (const auto &row : menu->rows) {
        if (row.kind == EWidget::DROPDOWN && row.widget != nullptr && row.widget[202] != 0) {
            menu->activeDropdown = row.widget;
            break;
        }
    }

    if (Word(menu->screen, 168) > 1) {
        menu->selections[static_cast<unsigned int>(menu->page)] = Word(menu->screen, 168);
        menu->scrollOffsets[static_cast<unsigned int>(menu->page)] = Float(ScrollControl(menu->screen), 724);
    }

    for (auto row = 0U; row < 21; ++row) {
        auto &item = menu->rows[row];
        if (item.widget == nullptr || item.kind == EWidget::BUTTON) {
            continue;
        }

        auto value = Value(state, row);
        if (item.kind == EWidget::SLIDER) {
            const auto steps = (item.maximum - item.minimum) / item.step;
            value = item.minimum + static_cast<unsigned int>(Float(item.widget, 80) * steps + 0.5f) * item.step;
        } else if (item.kind == EWidget::BOOLEAN) {
            value = item.widget[248] != 0;
        } else if (item.widget[202] == 0) {
            value = Word(item.widget, 208);
        }

        if (value != Value(state, row) && value >= item.minimum && value <= item.maximum) {
            SetValue(state, row, value);
            if (menu->page != state.page) {
                state.status[0] = 0;
                BuildRows(state, *menu);
            }

            RuntimeControls::Changed(state);
            break;
        }
    }
}

unsigned char *NativeMenus::ScrollControl(unsigned char *screen) {
    auto *pane = *reinterpret_cast<unsigned char **>(screen + 148);
    return *reinterpret_cast<unsigned char **>(pane + 124);
}

const char *NativeMenus::Help(const TState &state) {
    const auto *menu = static_cast<const TMenu *>(state.nativeMenu);
    for (auto row = 2U; row < 17; ++row) {
        const auto *widget = menu->rows[row].widget;
        if (widget == nullptr || widget[48] == 0) {
            continue;
        }

        switch (static_cast<ESetting>(SettingForRow(state.page, row))) {
        case ESetting::LOOK_X:
        case ESetting::LOOK_Y:
            return "Per-mode gain multiplies native sensitivity.";
        case ESetting::MODIFIER_TOGGLE:
            return "Toggle latches L-mode camera until pressed again.";
        case ESetting::CAMERA_CURVE:
            return "Precision: finer aim. Quick: faster small moves.";
        case ESetting::CAMERA_ACCELERATION:
            return "Extra speed beyond half stick travel. 0 = off.";
        case ESetting::PARTICLES:
            return "Reduced halves particle draw distance.";
        case ESetting::WEATHER:
            return "Hides rain/snow; weather simulation is unchanged.";
        case ESetting::MAP_RATE:
            return "Slower map redraws do not slow world updates.";
        case ESetting::LOW_MEMORY_TEXTURES:
            return "Smaller terrain mipmaps. Full restart required.";
        case ESetting::OVERLAY_SCALE:
            return "Text and graph size. Long lines shrink to fit.";
        case ESetting::FRAME_GRAPH:
            return "32 displayed frames. Guide = 33.3 ms; max 100.";
        case ESetting::RENDER_DISTANCE:
            return "16 blocks/chunk. Higher distances use more RAM.";
        case ESetting::CHUNK_RADIUS:
            return "1/2/3 = 9/25/49 nearby chunks; affects ticking.";
        case ESetting::MAP_RADIUS:
            return "Map update area, not refresh speed. Restart.";
        case ESetting::MESH_BUILDERS:
            return "Fewer builders save scratch memory. Restart.";
        default:
            break;
        }
    }

    const char *help[] = {
        "Shared keys are allowed. Unbind to disable.", "Menu binds do not change this page's controls.",
        "Curve/acceleration shared; X/Y gain per mode.", "Preview of the bottom screen's touch stick.",
        "Presets change visuals, not world simulation.", "Copy/reset affects the selected control mode.",
        "Largest M/G/S = largest free payload per heap."};
    return help[static_cast<unsigned int>(state.page)];
}

void NativeMenus::ForgetScreen(unsigned char *screen) {
    if (runtimeControls != nullptr && runtimeControls->nativeMenu != nullptr) {
        auto *menu = static_cast<TMenu *>(runtimeControls->nativeMenu);
        if (menu->screen == screen) {
            menu->screen = nullptr;
        }
    }
}

void NativeMenus::NativeInitializeOptions(unsigned char *screen) {
    NativeOriginalOptionsInitialize(screen);
    if (screen[280] != 2) {
        return;
    }

    auto *state = RuntimeControls::GetState();
    if (state == nullptr) {
        return;
    }

    auto *menu = GetMenu(*state);
    if (menu == nullptr) {
        return;
    }

    menu->screen = screen;
    state->screen = screen;
    state->options = static_cast<unsigned char *>(ControlsGetOptions(*reinterpret_cast<void **>(screen + 4)));
    state->status[0] = 0;
    auto *titles = *reinterpret_cast<TString **>(screen + 392);
    ControlsStringAssign(titles + 2, "Mod Settings");
    BuildRows(*state, *menu);
}

void NativeMenus::NativeOptionsBack(unsigned char *screen) {
    if (screen[280] == 2) {
        Word(screen, 284) = 0;
    }

    NativeOriginalOptionsBack(screen);
}

void NativeMenus::AddMenuEntry(unsigned char *screen, bool mainMenu) {
    TString label;
    unsigned int temporary[2];
    ControlsStringConstruct(&label, "Mod Settings", temporary);
    NativeAddMenuButton(screen, menuButtonId, &label, mainMenu ? 110 : 50, 0);
    ControlsStringRelease(&label);
    const auto *first = *reinterpret_cast<TReference **>(screen + 28);
    const auto *last = *reinterpret_cast<TReference **>(screen + 32);
    auto index = 0U;
    for (auto *entry = first; entry != last; ++entry, ++index) {
        Word(entry->object, 16) = (mainMenu ? 5 : 29) + index * 27;
        Word(entry->object, 24) = 25;
        if (mainMenu) {
            Word(entry->object, 20) = 200;
        }
    }

    if (mainMenu && screen[164] != 0) {
        //The native modal-close path adds every main-menu control to navigation.
        auto **navigation = reinterpret_cast<TReference **>(screen + 52);
        ControlsReleaseReference(--navigation[1]);
        (last - 1)->object[8] = 0;
    }
}

void NativeMenus::NativeInitializeMainMenu(unsigned char *screen) {
    NativeOriginalMainInitialize(screen);
    AddMenuEntry(screen, true);
}

void NativeMenus::NativeInitializePauseMenu(unsigned char *screen) {
    NativeOriginalPauseInitialize(screen);
    AddMenuEntry(screen, false);
}

int NativeMenus::NativeMainButtonEvent(unsigned char *screen, unsigned char *button) {
    if (Word(button, 120) == menuButtonId) {
        NativeOpenOptions(*reinterpret_cast<void **>(*reinterpret_cast<unsigned char **>(screen + 4) + 328), 2);
        return 1;
    }

    return NativeOriginalMainButton(screen, button);
}

int NativeMenus::NativePauseButtonEvent(unsigned char *screen, unsigned char *button) {
    if (Word(button, 120) == menuButtonId) {
        NativeOpenOptions(*reinterpret_cast<void **>(*reinterpret_cast<unsigned char **>(screen + 4) + 328), 2);
        return 1;
    }

    return NativeOriginalPauseButton(screen, button);
}

int NativeMenus::NativeDropdownKey(unsigned char *widget, void *client, int action) {
    const auto id = Word(widget, 120);
    if (id < firstButtonId || id >= firstButtonId + 21) {
        return NativeOriginalDropdownKey(widget, client, action);
    }

    auto *menu = runtimeControls == nullptr ? nullptr : static_cast<TMenu *>(runtimeControls->nativeMenu);
    if ((menu != nullptr && menu->activeDropdown != nullptr && menu->activeDropdown != widget) ||
        (widget[48] == 0 && widget[202] == 0)) {
        return 0;
    }

    const auto result = NativeOriginalDropdownKey(widget, client, action);
    if (menu != nullptr && widget[202] != 0) {
        //Keep the owner through the entire broadcast, including its closing event.
        menu->activeDropdown = widget;
    }

    return result;
}

void NativeMenus::NativeRenderOptions(unsigned char *screen, int x, int y, unsigned int flags) {
    NativeOriginalRender(screen, x, y, flags);
    const auto *state = runtimeControls;
    if ((flags & 0x40) == 0 || screen[280] != 2 || state == nullptr || state->screen != screen) {
        return;
    }

    struct TText {
        int references;
        unsigned int capacity;
        unsigned int length;
        char text[96];
    } text;
    text.references = -1;
    text.capacity = 95;
    text.length = 0;
    const auto *message = state->captureIndex >= 0 ? (state->captureReady ? state->status : "Release all buttons, then press your new bind.") :
        Help(*state);
    if (state->captureIndex < 0 && state->page == EPage::DISPLAY &&
        ControlSettings::GetSetting(state->config, ESetting::LOW_MEMORY_TEXTURES) != state->activeLowMemoryTextures) {
        message = "Restart pending: texture quality changed.";
    }
    if (state->captureIndex < 0 && state->page == EPage::DISPLAY && RuntimeWorld::NeedsRestart(state->config)) {
        message = "Restart pending: world/chunk settings changed.";
    }
    RuntimeControls::Append(text.text, message);
    while (text.text[text.length] != 0) {
        ++text.length;
    }

    const auto *value = text.text;
    const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    DebugOverlay::DrawScaledText(*reinterpret_cast<void **>(screen + 100), &value, white, 6.0f, 6.0f, 1.0f);
    if (state->page == EPage::TOUCH) {
        const auto settings = RuntimeControls::GetTouchSettings(*state);
        const auto alpha = ControlSettings::GetSetting(state->config, ESetting::TOUCH_OPACITY) * 0.01f;
        const float rim[] = {0.82f, 0.88f, 0.92f, alpha};
        const float inside[] = {0.08f, 0.12f, 0.18f, alpha * 0.65f};
        const float deadZone[] = {1.0f, 0.7f, 0.2f, 0.75f};
        float previous[4];
        for (auto index = 0; index < 4; ++index) {
            previous[index] = touchGuiColorState[index];
        }

        TouchSetGuiColor(touchGuiColorState, touchWhiteColor);
        TouchBegin(touchTessellator, 288);
        TouchStick::DrawDisk(settings.x + 40, settings.y, settings.radius, rim);
        TouchStick::DrawDisk(settings.x + 40, settings.y, settings.radius - 2, inside);
        TouchStick::DrawDisk(settings.x + 40, settings.y, settings.deadZone, deadZone);
        TouchDraw(touchTessellator, touchGuiMaterial);
        TouchSetGuiColor(touchGuiColorState, previous);
    }
}
