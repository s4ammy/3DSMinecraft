#pragma once

#include "ControlSettings.h"

namespace RuntimePrompts {
    unsigned int BindingForGlyph(const ControlSettings::TConfig &config, unsigned int glyph, bool gameplay);
    int GlyphForButton(unsigned int buttons);
    extern "C" void DrawRuntimePrompt(unsigned char *helper, int x, int y, unsigned int glyph, const char **label);
}
