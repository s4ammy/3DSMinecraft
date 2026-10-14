#include "RuntimePrompts.h"

namespace RuntimePrompts {
    constexpr unsigned short glyphButtons[] = {1, 2, 0x400, 0x800, 0x40, 0x80, 0x20, 0x10, 0x200, 0x100, 0x4000, 0x8000};
    constexpr unsigned char gameActions[] = {0, 3, 4, 5, 8, 9, 6, 7, 1, 2, 12, 13};
    constexpr unsigned char menuActions[] = {15, 16, 17, 18, 21, 22, 23, 24, 19, 20, 27, 28};
}

unsigned int RuntimePrompts::BindingForGlyph(const ControlSettings::TConfig &config, unsigned int glyph, bool gameplay) {
    if (glyph >= sizeof(gameActions) || static_cast<unsigned int>(config.mode) >= ControlSettings::modeCount) {
        return 0;
    }

    return config.profiles[static_cast<unsigned int>(config.mode)].bindings[(gameplay ? gameActions : menuActions)[glyph]];
}

int RuntimePrompts::GlyphForButton(unsigned int buttons) {
    for (auto index = 0U; index < sizeof(glyphButtons) / sizeof(glyphButtons[0]); ++index) {
        if (glyphButtons[index] == buttons) {
            return static_cast<int>(index);
        }
    }

    return -1;
}
