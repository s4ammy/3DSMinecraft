#include "ui/NativeUi.h"

extern "C" {
void ControlsStringAssign(RuntimeControls::TString *text, const char *value);
void ControlsCreateButton(RuntimeControls::TReference *result, void *screen, const char *label, unsigned int id, int iconX, int iconY);
void NativeCreateSlider(RuntimeControls::TReference *result, void *screen, const void *key);
void NativeCreateBoolean(RuntimeControls::TReference *result, void *screen, const void *key);
void NativeCreateDropdown(RuntimeControls::TReference *result, void *screen, const void *key,
    const char *const *choices, unsigned int count, unsigned int id, bool enabled);
void NativeDropdownSelect(void *widget, unsigned int index);
}

namespace NativeUi {
    constexpr const char *modeNames[] = {"L + Circle Pad", "Circle Pad Pro", "Touch stick", "ABXY camera"};
    constexpr const char *pageNames[] = {"Gameplay binds", "Menu binds", "Camera", "Touch stick", "Display", "Binding tools", "Overlay"};
    constexpr const char *overlayNames[] = {"Off", "FPS", "Full"};
    constexpr const char *particleNames[] = {"Normal", "Reduced", "Hidden"};
    constexpr const char *mapNames[] = {"Normal", "4 Hz", "1 Hz", "Paused"};
    constexpr const char *cornerNames[] = {"Top left", "Top right", "Bottom left", "Bottom right"};
    constexpr const char *curveNames[] = {"Linear", "Precision", "Quick"};
    constexpr const char *presetNames[] = {"Custom", "Quality", "Balanced", "Low memory"};
    constexpr const char *countNames[] = {"1", "2", "3", "4"};
    constexpr const char *chunkNames[] = {"1 (9 chunks)", "2 (25 chunks)", "3 (49 chunks)"};
    constexpr const char *radiusNames[] = {"8", "16", "24", "32"};
    constexpr const char *const *choices[] = {modeNames, pageNames, overlayNames, particleNames, mapNames,
        cornerNames, curveNames, presetNames, countNames, chunkNames, radiusNames};
}

void NativeUi::Create(RuntimeControls::TReference &reference, TRow &row, unsigned char *screen, unsigned int id) {
    const auto &spec = *row.spec;
    if (spec.kind == EWidget::SLIDER) {
        NativeCreateSlider(&reference, screen, reinterpret_cast<const void *>(0xa31c64));
    } else if (spec.kind == EWidget::BOOLEAN) {
        NativeCreateBoolean(&reference, screen, reinterpret_cast<const void *>(0xa31c5c));
    } else if (spec.kind == EWidget::DROPDOWN) {
        NativeCreateDropdown(&reference, screen, reinterpret_cast<const void *>(0xa31c8c),
            choices[static_cast<unsigned int>(spec.choices)], spec.maximum + 1, id, false);
        //Use base option-row behavior without world-setting synchronization.
        Word(reference.object, 0) = 0x99d7e0;
        Word(reference.object, 4) = 0x99d884;
    } else {
        ControlsCreateButton(&reference, screen, "Controls", id, 240, 144);
    }

    const auto *children = *reinterpret_cast<const RuntimeControls::TReference **>(reference.object + 56);
    row.widget = children[0].object;
    if (spec.kind != EWidget::BUTTON) {
        Word(reference.object, 96) = reinterpret_cast<unsigned int>(&row.descriptor);
        if (spec.kind == EWidget::SLIDER) {
            Word(row.widget, 108) = 0;
            Word(row.widget, 20) = 240;
        } else if (spec.kind == EWidget::BOOLEAN) {
            Word(row.widget, 244) = 0;
        }
    }
}

unsigned int NativeUi::ReadValue(const TRow &row) {
    const auto &spec = *row.spec;
    if (spec.kind == EWidget::SLIDER) {
        const auto steps = (spec.maximum - spec.minimum) / spec.step;
        return spec.minimum + static_cast<unsigned int>(Float(row.widget, 80) * steps + 0.5f) * spec.step;
    }

    if (spec.kind == EWidget::BOOLEAN) {
        return row.widget[248] != 0;
    }

    return Word(row.widget, 208);
}

void NativeUi::Update(TRow &row, const char *label, unsigned int value) {
    if (!RuntimeControls::Equals(row.descriptor.label.data, label)) {
        ControlsStringAssign(&row.descriptor.label, label);
    }

    if (row.widget == nullptr) {
        return;
    }

    const auto &spec = *row.spec;
    if (spec.kind == EWidget::SLIDER) {
        Float(row.widget, 80) = static_cast<float>(value - spec.minimum) / (spec.maximum - spec.minimum);
    } else if (spec.kind == EWidget::BOOLEAN) {
        row.widget[248] = value != 0;
    } else if (row.widget[202] == 0 && Word(row.widget, 208) != value) {
        NativeDropdownSelect(row.widget, value);
    }
}
