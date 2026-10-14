#include "RuntimePrompts.h"
#include "RuntimeControls.h"
#include "DebugOverlay.h"

extern "C" {
extern RuntimeControls::TState *runtimeControls;
void ControlsOriginalPrompt(void *helper, int x, int y, unsigned int glyph, const char **label);
void ControlsLocalize(RuntimeControls::TString *result, const char **key, const void *substitutions);
void ControlsStringRelease(RuntimeControls::TString *text);
int ControlsMeasureText(void *font, const char **text, unsigned int flags, float scale);
void ControlsDrawText(void *font, const char **text, const float *color, unsigned int shadow, unsigned int flags, float x, float y, float shadowOffset);
}

void RuntimePrompts::DrawRuntimePrompt(unsigned char *helper, int x, int y, unsigned int glyph, const char **label) {
    const auto *state = runtimeControls;
    if (state == nullptr || glyph >= 12 || RuntimeControls::IsControlsPage(*state)) {
        ControlsOriginalPrompt(helper, x, y, glyph, label);
        return;
    }

    const auto buttons = BindingForGlyph(state->config, glyph, TouchStick::IsGameplay(state->touch.hud));
    const auto mappedGlyph = GlyphForButton(buttons);
    if (mappedGlyph >= 0) {
        ControlsOriginalPrompt(helper, x, y, static_cast<unsigned int>(mappedGlyph), label);
        return;
    }

    //Keep the native 18-pixel slot, including adjacent paired prompts.
    struct TPromptText {
        int references;
        unsigned int capacity;
        unsigned int length;
        char text[96];
    };
    TPromptText binding;
    binding.references = -1;
    binding.capacity = 95;
    binding.length = 0;
    if (buttons == 0) {
        binding.text[0] = '-';
        binding.text[1] = 0;
    } else {
        ControlSettings::DescribeBinding(buttons, binding.text);
    }

    while (binding.text[binding.length] != 0) {
        ++binding.length;
    }

    auto *client = *reinterpret_cast<unsigned char **>(helper + 56);
    auto *font = *reinterpret_cast<void **>(client + 92);
    const float color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    const char *text = binding.text;
    const auto width = ControlsMeasureText(font, &text, 0, 1.0f);
    const auto scale = width > 16 ? 16.0f / static_cast<float>(width) : 1.0f;
#ifdef MC3DS_NATIVE_MENU
    DebugOverlay::DrawScaledText(font, &text, color, static_cast<float>(x), static_cast<float>(y + 3), scale);
#else
    ControlsDrawText(font, &text, color, 1, 0, static_cast<float>(x), static_cast<float>(y + 3), scale);
#endif
    RuntimeControls::TString localized;
    ControlsLocalize(&localized, label, nullptr);
    text = localized.data;
    ControlsDrawText(font, &text, color, 1, 0, static_cast<float>(x + 18), static_cast<float>(y + 3), 1.0f);
    ControlsStringRelease(&localized);
}
