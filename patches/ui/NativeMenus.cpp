#include "ui/NativeMenus.h"
#include "ui/NativeUi.h"
#include "world/RuntimeWorld.h"
#include "overlay/DebugOverlay.h"

extern "C" {
extern RuntimeControls::TState *runtimeControls;
void *ControlsAllocate(unsigned int size);
void ControlsStringConstruct(RuntimeControls::TString *text, const char *value, void *temporary);
void ControlsStringAssign(RuntimeControls::TString *text, const char *value);
void ControlsStringRelease(RuntimeControls::TString *text);
void ControlsAppendReference(void *vector, const RuntimeControls::TReference *reference);
void ControlsReleaseReference(RuntimeControls::TReference *reference);
void *ControlsGetOptions(void *client);
void ControlsSetFloat(void *options, const void *key, float value);
void ControlsSetBoolean(void *options, const void *key, unsigned int value);
void NativeClearReferences(void *vector);
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
int NativeDropdownMove(void *widget, int direction);
int NativeOriginalNavigation(void *screen, int event, int direction);
void NativeMapSelection(void *screen);
void NativeUpdateSelection(void *screen, unsigned int index);
void NativeSetScroll(void *scroll, float value);
void NativeOriginalRender(void *screen, int x, int y, unsigned int flags);
void NativeLayoutScrollContainer(void *container);
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
    using NativeUi::EWidget;
    using NativeUi::EChoices;
    using NativeUi::TRow;
    using NativeUi::TSpec;
    using NativeUi::Word;
    using NativeUi::Float;

    struct TEntry {
        EPage page;
        unsigned char row;
        TSpec spec;
    };

    constexpr TSpec buttonSpec = {0, 1, 1, EWidget::BUTTON, ESetting::COUNT, EChoices::MODE};
    constexpr TSpec modeSpec = {0, 3, 1, EWidget::DROPDOWN, ESetting::COUNT, EChoices::MODE};
    constexpr TSpec pageSpec = {0, 6, 1, EWidget::DROPDOWN, ESetting::COUNT, EChoices::PAGE};
    constexpr TEntry entries[] = {
        {EPage::CAMERA, 2, {0, 100, 1, EWidget::SLIDER, ESetting::COUNT, EChoices::MODE}},
        {EPage::CAMERA, 3, {0, 1, 1, EWidget::BOOLEAN, ESetting::COUNT, EChoices::MODE}},
        {EPage::CAMERA, 4, {0, 1, 1, EWidget::BOOLEAN, ESetting::COUNT, EChoices::MODE}},
        {EPage::CAMERA, 5, {25, 200, 5, EWidget::SLIDER, ESetting::LOOK_X, EChoices::MODE}},
        {EPage::CAMERA, 6, {25, 200, 5, EWidget::SLIDER, ESetting::LOOK_Y, EChoices::MODE}},
        {EPage::CAMERA, 7, {0, 1, 1, EWidget::BOOLEAN, ESetting::MODIFIER_TOGGLE, EChoices::MODE}},
        {EPage::CAMERA, 14, {0, 2, 1, EWidget::DROPDOWN, ESetting::CAMERA_CURVE, EChoices::CURVE}},
        {EPage::CAMERA, 15, {0, 100, 1, EWidget::SLIDER, ESetting::CAMERA_ACCELERATION, EChoices::MODE}},
        {EPage::TOUCH, 2, {72, 248, 1, EWidget::SLIDER, ESetting::TOUCH_X, EChoices::MODE}},
        {EPage::TOUCH, 3, {72, 168, 1, EWidget::SLIDER, ESetting::TOUCH_Y, EChoices::MODE}},
        {EPage::TOUCH, 4, {28, 68, 1, EWidget::SLIDER, ESetting::TOUCH_RADIUS, EChoices::MODE}},
        {EPage::TOUCH, 5, {0, 15, 1, EWidget::SLIDER, ESetting::TOUCH_DEAD_ZONE, EChoices::MODE}},
        {EPage::TOUCH, 6, {0, 100, 1, EWidget::SLIDER, ESetting::TOUCH_OPACITY, EChoices::MODE}},
        {EPage::TOUCH, 7, {0, 1, 1, EWidget::BOOLEAN, ESetting::TOUCH_HIDE_IDLE, EChoices::MODE}},
        {EPage::DISPLAY, 2, {0, 2, 1, EWidget::DROPDOWN, ESetting::OVERLAY, EChoices::OVERLAY}},
        {EPage::DISPLAY, 3, {0, 2, 1, EWidget::DROPDOWN, ESetting::PARTICLES, EChoices::PARTICLES}},
        {EPage::DISPLAY, 4, {0, 1, 1, EWidget::BOOLEAN, ESetting::WEATHER, EChoices::MODE}},
        {EPage::DISPLAY, 5, {0, 3, 1, EWidget::DROPDOWN, ESetting::MAP_RATE, EChoices::MAP_RATE}},
        {EPage::DISPLAY, 6, {0, 1, 1, EWidget::BOOLEAN, ESetting::LOW_MEMORY_TEXTURES, EChoices::MODE}},
        {EPage::DISPLAY, 9, {0, 3, 1, EWidget::DROPDOWN, ESetting::COUNT, EChoices::PRESET}},
        {EPage::DISPLAY, 10, {0, 2, 1, EWidget::DROPDOWN, ESetting::RENDER_DISTANCE, EChoices::COUNT}},
        {EPage::DISPLAY, 11, {0, 2, 1, EWidget::DROPDOWN, ESetting::CHUNK_RADIUS, EChoices::CHUNKS}},
        {EPage::DISPLAY, 12, {0, 3, 1, EWidget::DROPDOWN, ESetting::MAP_RADIUS, EChoices::RADIUS}},
        {EPage::DISPLAY, 13, {0, 3, 1, EWidget::DROPDOWN, ESetting::MESH_BUILDERS, EChoices::COUNT}},
        {EPage::TOOLS, 5, modeSpec},
        {EPage::TOOLS, 7, {0, 1, 1, EWidget::BOOLEAN, ESetting::CONFLICT_HINTS, EChoices::MODE}},
        {EPage::OVERLAY, 2, {0, 3, 1, EWidget::DROPDOWN, ESetting::OVERLAY_POSITION, EChoices::CORNER}},
        {EPage::OVERLAY, 3, {50, 125, 5, EWidget::SLIDER, ESetting::OVERLAY_SCALE, EChoices::MODE}},
        {EPage::OVERLAY, 4, {0, 100, 1, EWidget::SLIDER, ESetting::OVERLAY_OPACITY, EChoices::MODE}},
        {EPage::OVERLAY, 5, {0, 1, 1, EWidget::BOOLEAN, ESetting::FRAME_GRAPH, EChoices::MODE}}
    };

    struct TMenu {
        unsigned char *screen;
        EPage page;
        bool scrollLayoutPending;
        TRow rows[21];
        unsigned int selections[7];
        float scrollOffsets[7];
        unsigned char *activeDropdown;
    };

    static_assert(sizeof(TMenu) == 404, "Native menu descriptor budget changed");
    static_assert(__builtin_offsetof(TMenu, scrollLayoutPending) == 5, "Scroll deferral must use existing padding");
    static_assert(__builtin_offsetof(TMenu, rows) == 8, "Native row layout changed");
    static_assert(sizeof(TEntry) == 8, "Native control specification budget changed");

    void (*const scrollListenerVtable[])(unsigned char *) = {nullptr, nullptr, NativeScrollChanged};

    constexpr unsigned int menuButtonId = 0x2000;
    constexpr unsigned int firstButtonId = 0x1000;
    TMenu *GetMenu(TState &state);
    const TSpec &SpecForRow(EPage page, unsigned int row);
    int SettingForRow(EPage page, unsigned int row);
    unsigned int Value(const TState &state, unsigned int row);
    void SetValue(TState &state, unsigned int row, unsigned int value);
    void ConfigureRow(TRow &item, EPage page, unsigned int row);
    void BuildRows(TState &state, TMenu &menu);
    void AddMenuEntry(unsigned char *screen, bool mainMenu);
    unsigned char *ScrollControl(unsigned char *screen);
    const char *Help(const TState &state);
}

NativeMenus::TMenu *NativeMenus::GetMenu(TState &state) {
    if (state.nativeMenu == nullptr) {
        auto *menu = static_cast<TMenu *>(ControlsAllocate(sizeof(TMenu)));
        if (menu == nullptr) {
            return nullptr;
        }

        menu->screen = nullptr;
        menu->activeDropdown = nullptr;
        menu->scrollLayoutPending = false;
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
    const auto setting = SpecForRow(page, row).setting;
    return setting == ESetting::COUNT ? -1 : static_cast<int>(setting);
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

const NativeUi::TSpec &NativeMenus::SpecForRow(EPage page, unsigned int row) {
    if (row < 2) {
        return row == 0 ? modeSpec : pageSpec;
    }

    for (const auto &entry : entries) {
        if (entry.page == page && entry.row == row) {
            return entry.spec;
        }
    }

    return buttonSpec;
}

void NativeMenus::ConfigureRow(TRow &item, EPage page, unsigned int row) {
    item.widget = nullptr;
    item.spec = &SpecForRow(page, row);
}

bool NativeMenus::UpdateRow(TState &state, unsigned int row, const char *label) {
    auto *menu = static_cast<TMenu *>(state.nativeMenu);
    if (menu == nullptr || menu->screen != state.screen) {
        return false;
    }

    auto &item = menu->rows[row];
    if (item.spec->kind == EWidget::BUTTON) {
        return false;
    }

    NativeUi::Update(item, label, Value(state, row));
    return true;
}

void NativeMenus::BuildRows(TState &state, TMenu &menu) {
    auto *screen = menu.screen;
    menu.activeDropdown = nullptr;
    menu.scrollLayoutPending = false;
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
        NativeUi::Create(reference, item, screen, firstButtonId + row);
        if (item.spec->kind == EWidget::BUTTON) {
            state.buttons[row] = item.widget;
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
        if (row.widget != nullptr && row.spec->kind == EWidget::DROPDOWN && row.widget[202] != 0) {
            menu->activeDropdown = row.widget;
            break;
        }
    }

    if (menu->activeDropdown == nullptr && Word(menu->screen, 168) > 1) {
        menu->selections[static_cast<unsigned int>(menu->page)] = Word(menu->screen, 168);
        menu->scrollOffsets[static_cast<unsigned int>(menu->page)] = Float(ScrollControl(menu->screen), 724);
    }

    for (auto row = 0U; row < 21; ++row) {
        auto &item = menu->rows[row];
        if (item.widget == nullptr || item.spec->kind == EWidget::BUTTON ||
            (item.spec->kind == EWidget::DROPDOWN && item.widget[202] != 0)) {
            continue;
        }

        const auto value = NativeUi::ReadValue(item);
        if (value != Value(state, row) && value >= item.spec->minimum && value <= item.spec->maximum) {
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
            menu->activeDropdown = nullptr;
            menu->scrollLayoutPending = false;
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
    auto *container = *reinterpret_cast<unsigned char **>(screen + 148);
    Word(container, 96) = reinterpret_cast<unsigned int>(scrollListenerVtable);
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

int NativeMenus::NativeOptionsNavigation(unsigned char *screen, int event, int direction) {
    const auto *state = runtimeControls;
    if (state != nullptr && state->screen == screen && screen[280] == 2) {
        const auto *menu = static_cast<const TMenu *>(state->nativeMenu);
        if (state->captureIndex >= 0) {
            return 1;
        }

        if (menu != nullptr && menu->screen == screen) {
            for (const auto &row : menu->rows) {
                if (row.widget != nullptr && row.spec->kind == EWidget::DROPDOWN && row.widget[202] != 0) {
                    if (event == 1) {
                        NativeDropdownMove(row.widget, direction);
                    }

                    return 1;
                }
            }
        }
    }

    return NativeOriginalNavigation(screen, event, direction);
}

void NativeMenus::NativeScrollChanged(unsigned char *listener) {
    //Only our container installs this listener. Layout must wait for scroll to return.
    static_cast<TMenu *>(runtimeControls->nativeMenu)->scrollLayoutPending = true;
}

void NativeMenus::FlushScrollLayout(unsigned char *screen) {
    auto *menu = static_cast<TMenu *>(runtimeControls->nativeMenu);
    if (menu->scrollLayoutPending) {
        menu->scrollLayoutPending = false;
        NativeLayoutScrollContainer(*reinterpret_cast<void **>(screen + 148));
    }
}

void NativeMenus::NativeRenderOptions(unsigned char *screen, int x, int y, unsigned int flags) {
    const auto *state = runtimeControls;
    const auto customScreen = screen[280] == 2 && state != nullptr && state->screen == screen && state->nativeMenu != nullptr;
    if (customScreen) {
        //Render entry is outside the pane's active input and scroll callback stack.
        FlushScrollLayout(screen);
    }

    NativeOriginalRender(screen, x, y, flags);
    if ((flags & 0x40) == 0 || !customScreen) {
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
