#include "CompactBindings.h"
#include "RuntimeControls.h"

extern "C" {
void ControlsStringConstruct(RuntimeControls::TString *text, const char *value, void *temporary);
void ControlsStringRelease(RuntimeControls::TString *text);
void ControlsAppendDictation(void *vector, const void *record);
}

namespace CompactBindings {
    struct TDictationBinding {
        const char *action;
        const char *phrase;
        unsigned int event;
        unsigned int thresholdBits;
    };

    constexpr TDictationBinding dictationBindings[] = {
        {"button.holo_zoom_in", "closer", 3, 1051931443U},
        {"button.holo_zoom_in", "closer", 2, 1051931443U},
        {"button.holo_zoom_out", "back", 3, 1051931443U},
        {"button.holo_zoom_out", "back", 2, 1051931443U},
        {"button.holo_zoom_max", "close up", 3, 1051931443U},
        {"button.holo_zoom_max", "close up", 2, 1051931443U},
        {"button.holo_zoom_min", "overview", 3, 1048576000U},
        {"button.holo_zoom_min", "overview", 2, 1048576000U},
        {"button.holo_lightningbolt", "lightningstrike", 3, 1045220557U},
        {"button.holo_lightningbolt", "lightning strike", 3, 1045220557U},
        {"button.holo_lightningbolt", "lightingstrike", 3, 1045220557U},
        {"button.holo_lightningbolt", "lighting strike", 3, 1045220557U},
        {"button.holo_lightningbolt", "lightningstrike", 2, 1045220557U},
        {"button.holo_lightningbolt", "lightning strike", 2, 1045220557U},
        {"button.holo_lightningbolt", "lightingstrike", 2, 1045220557U},
        {"button.holo_lightningbolt", "lighting strike", 2, 1045220557U},
        {"button.holo_raiseworld", "raise", 3, 1053609165U},
        {"button.holo_raiseworld", "raise", 2, 1053609165U},
        {"button.holo_lowerworld", "lower", 3, 1053609165U},
        {"button.holo_lowerworld", "lower", 2, 1053609165U},
        {"button.holo_follow", "follow", 2, 1048576000U},
        {"button.holo_follow", "follow player", 2, 1048576000U},
        {"button.holo_stopfollow", "stop following", 3, 1048576000U},
        {"button.holo_follownext", "next player", 3, 1048576000U},
        {"button.advancetime_sunrise", "time sunrise", 3, 1051931443U},
        {"button.advancetime_sunrise", "time sunrise", 2, 1051931443U},
        {"button.advancetime_morning", "time morning", 3, 1051931443U},
        {"button.advancetime_morning", "time morning", 2, 1051931443U},
        {"button.advancetime_afternoon", "time afternoon", 3, 1051931443U},
        {"button.advancetime_afternoon", "time afternoon", 2, 1051931443U},
        {"button.advancetime_evening", "time evening", 3, 1051931443U},
        {"button.advancetime_evening", "time evening", 2, 1051931443U},
        {"button.advancetime_midnight", "time midnight", 3, 1051931443U},
        {"button.advancetime_midnight", "time midnight", 2, 1051931443U},
        {"button.holo_playspace", "playspace", 3, 1050253722U},
        {"button.holo_playspace", "play space", 3, 1050253722U},
        {"button.holo_playspace", "playspace", 2, 1050253722U},
        {"button.holo_playspace", "play space", 2, 1050253722U},
        {"button.holo_playspace", "playspace", 1, 1050253722U},
        {"button.holo_playspace", "play space", 1, 1050253722U},
        {"button.holo_placescreen", "place screen", 0, 1045220557U},
        {"button.holo_placescreen", "playscreen", 0, 1045220557U},
        {"button.holo_placeviewer", "place world", 0, 1045220557U},
        {"button.holo_placeviewer", "playsworld", 0, 1045220557U},
        {"button.holo_goback", "go back", 0, 1048576000U},
        {"button.holo_goback", "goback", 0, 1048576000U},
        {"button.holo_viewermode", "create world", 0, 1048576000U},
        {"button.holo_viewermode", "create world", 1, 1048576000U},
        {"button.holo_viewermode", "create world", 4, 1048576000U},
        {"button.holo_realitymode", "reality mode", 1, 1045220557U},
        {"button.holo_realitymode", "reality mode", 3, 1045220557U},
        {"button.holo_realitymode", "reality mode", 2, 1045220557U},
        {"button.holo_realitymode", "wrap around mode", 1, 1045220557U},
        {"button.holo_realitymode", "wrap around mode", 3, 1045220557U},
        {"button.holo_realitymode", "wrap around mode", 2, 1045220557U},
        {"button.holo_realitymode_VR", "wrap around mode", 4, 1045220557U},
        {"button.holo_realitymode_reality", "reality mode", 4, 1045220557U},
        {"button.holo_screenmode", "create screen", 0, 1045220557U},
        {"button.holo_screenmode", "create screen", 3, 1045220557U},
        {"button.holo_screenmode", "create screen", 2, 1045220557U},
        {"button.holo_screenmode", "create screen", 4, 1045220557U},
        {"button.holo_autoalign", "auto align", 0, 1048576000U},
        {"button.holo_autoalign", "autoalign", 0, 1048576000U},
        {"button.holo_autoalign", "autoaline", 0, 1048576000U},
        {"button.holo_pan", "explore", 3, 1048576000U},
        {"button.holo_pan", "explore", 2, 1048576000U},
        {"button.holo_pan", "grab", 0, 1048576000U},
        {"button.holo_pan", "move", 0, 1048576000U},
        {"button.holo_rotate", "rotate", 0, 1048576000U},
        {"button.holo_rotate", "turn", 0, 1048576000U},
        {"button.holo_tilt", "tilt", 0, 1048576000U},
        {"button.holo_zoom_mode", "resize", 1, 1050253722U},
        {"button.holo_zoom_mode", "resize", 0, 1050253722U},
        {"button.holo_zoom_mode", "zoom", 3, 1050253722U},
        {"button.holo_zoom_mode", "zoom", 2, 1050253722U},
        {"button.holo_pivotleft", "pivot left", 3, 1050253722U},
        {"button.holo_pivotleft", "pivot left", 2, 1050253722U},
        {"button.holo_pivotright", "pivot right", 3, 1050253722U},
        {"button.holo_pivotright", "pivot right", 2, 1050253722U},
        {"button.holo_screen_2d", "two dee", 1, 1050253722U},
        {"button.holo_screen_3d", "three dee", 1, 1050253722U},
        {"button.holo_small_screen", "small tee vee", 0, 1048576000U},
        {"button.holo_small_screen", "small tee vee", 1, 1048576000U},
        {"button.holo_small_screen", "small tee vee", 4, 1048576000U},
        {"button.holo_medium_screen", "medium tee vee", 0, 1048576000U},
        {"button.holo_medium_screen", "medium tee vee", 1, 1048576000U},
        {"button.holo_medium_screen", "medium tee vee", 4, 1048576000U},
        {"button.holo_large_screen", "large tee vee", 0, 1048576000U},
        {"button.holo_large_screen", "large tee vee", 1, 1048576000U},
        {"button.holo_large_screen", "large tee vee", 4, 1048576000U},
        {"button.holo_huge_screen", "huge tee vee", 0, 1048576000U},
        {"button.holo_huge_screen", "huge tee vee", 1, 1048576000U},
        {"button.holo_huge_screen", "huge tee vee", 4, 1048576000U},
        {"button.holo_mark", "mark that", 3, 1048576000U},
        {"button.holo_mark", "mark that", 2, 1048576000U},
        {"button.holo_debug_toggle_lsr", "debug l s r", 2, 1048576000U},
        {"button.holo_debug_toggle_lsr", "debug l s r", 3, 1048576000U},
        {"button.holo_debug_toggle_lsr", "debug l s r", 1, 1048576000U},
        {"button.holo_debug_toggle_lsr", "debug l s r", 0, 1048576000U},
        {"button.holo_debug_toggle_anchors", "debug anchors", 2, 1048576000U},
        {"button.holo_debug_toggle_anchors", "debug anchors", 3, 1048576000U},
        {"button.holo_debug_toggle_anchors", "debug anchors", 1, 1048576000U},
        {"button.holo_debug_toggle_anchors", "debug anchors", 0, 1048576000U},
        {"button.holo_debug_toggle_thirdperson", "third person", 4, 1048576000U},
    };
}

void CompactBindings::BuildDictationBindings(void *, void *vector) {
    for (const auto &binding : dictationBindings) {
        struct TRecord {
            RuntimeControls::TString action;
            RuntimeControls::TString phrase;
            unsigned int event;
            unsigned int thresholdBits;
        } record;
        unsigned int temporary;
        ControlsStringConstruct(&record.action, binding.action, &temporary);
        ControlsStringConstruct(&record.phrase, binding.phrase, &temporary);
        record.event = binding.event;
        record.thresholdBits = binding.thresholdBits;
        ControlsAppendDictation(vector, &record);
        ControlsStringRelease(&record.phrase);
        ControlsStringRelease(&record.action);
    }
}
