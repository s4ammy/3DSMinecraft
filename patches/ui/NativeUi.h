#pragma once

#include "controls/RuntimeControls.h"

namespace NativeUi {
    enum class EWidget : unsigned char {
        BUTTON,
        SLIDER,
        BOOLEAN,
        DROPDOWN
    };

    enum class EChoices : unsigned char {
        MODE,
        PAGE,
        OVERLAY,
        PARTICLES,
        MAP_RATE,
        CORNER,
        CURVE,
        PRESET,
        COUNT,
        CHUNKS,
        RADIUS
    };

    struct TSpec {
        unsigned char minimum;
        unsigned char maximum;
        unsigned char step;
        EWidget kind;
        ControlSettings::ESetting setting;
        EChoices choices;
    };

    struct TDescriptor {
        unsigned int unused;
        RuntimeControls::TString label;
    };

    struct TRow {
        TDescriptor descriptor;
        unsigned char *widget;
        const TSpec *spec;
    };

    unsigned int &Word(unsigned char *object, unsigned int offset);
    float &Float(unsigned char *object, unsigned int offset);
    void Create(RuntimeControls::TReference &reference, TRow &row, unsigned char *screen, unsigned int id);
    unsigned int ReadValue(const TRow &row);
    void Update(TRow &row, const char *label, unsigned int value);
}

inline unsigned int &NativeUi::Word(unsigned char *object, unsigned int offset) {
    return *reinterpret_cast<unsigned int *>(object + offset);
}

inline float &NativeUi::Float(unsigned char *object, unsigned int offset) {
    return *reinterpret_cast<float *>(object + offset);
}
