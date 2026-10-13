#include "CompactBindings.h"
#include "RuntimeControls.h"

extern "C" {
    void ControlsRegisterBinding(void *vector, const char *name, const unsigned char *key, const unsigned char *enabled);
    void ControlsStringConstruct(RuntimeControls::TString *text, const char *value, void *temporary);
    void ControlsStringRelease(RuntimeControls::TString *text);
    void ControlsAppendModifiedBinding(void *vector, const void *record);
}

namespace CompactBindings {
    constexpr TBinding sharedBindings[] = {
        {"button.slot1", 49}, {"button.slot2", 50}, {"button.slot3", 51}, {"button.slot4", 52},
        {"button.slot5", 53}, {"button.slot6", 54}, {"button.slot7", 55}, {"button.slot8", 56},
        {"button.slot9", 57}, {"button.slot0", 48}, {"button.shift", 16}, {"button.control", 17},
        {"button.toggle_livingroom", 67}, {"button.menu_vr_realign", 78}, {"button.simulate_touch", 122},
        {"button.force_ui_pocket", 80}, {"button.reload_ui_toggle", 82}, {"button.ui_debug_toggle", 68},
        {"button.pagedown", 34}, {"button.pageup", 33}
    };
    constexpr TBinding commonBindings[] = {
        {"button.inventory", 69}, {"button.chat", 84}, {"button.build_or_interact", 85},
        {"button.down", 83}, {"button.right", 68}, {"button.render_distance", 70},
        {"button.destroy_or_attack", 88}, {"button.hide_gui", 112}, {"button.jump", 32},
        {"button.sprint", 17}, {"button.pause", 27}, {"button.sneak", 16},
        {"button.third_person_view", 116}, {"button.debug", 114}, {"button.console", 191},
        {"button.redstone_log", 89}, {"button.debug_texel_aa", 86}, {"button.advanced_culling", 74},
        {"button.day_cycle_active", 79}, {"button.drop_all_items", 73}, {"button.game_mode", 66},
        {"button.low_memory_warning", 115}, {"button.memory_stats", 119}, {"button.player_mob_update", 72},
        {"button.render_debug", 114}, {"button.suspend_resume", 120}, {"button.sim_time_unpause", 36},
        {"button.sim_time_pause", 35}, {"button.sim_time_scale_up", 33}, {"button.sim_time_scale_down", 34},
        {"button.take_screenshot", 19}, {"button.time_step_forward", 80}, {"button.set_spawn", 48},
        {"button.no_clip", 113}, {"button.biome_display", 45}, {"button.decrement_max_culling_steps", 75},
        {"button.fly_fast", 118}, {"button.frame_timer_display", 117}, {"button.increment_max_culling_steps", 76},
        {"button.next_avc_stat", 77}, {"button.debug_show_chunk_map", 77}
    };
    constexpr const char *layoutNames[] = {"button.mobeffects", "button.drop", "button.up", "button.left"};
    constexpr unsigned char layoutKeys[][4] = {{90, 81, 87, 65}, {87, 65, 90, 81}, {87, 81, 90, 65}};
    constexpr const char *modifiedBindings[][2] = {
        {"button.force_screens_type_pocket", "button.force_ui_pocket"},
        {"button.reload_ui_definitions", "button.reload_ui_toggle"},
        {"button.new_ui_debug", "button.ui_debug_toggle"},
        {"button.pick_debug_object", "button.mouse_pick_debug_object"},
        {"button.resize_down", "button.pagedown"}, {"button.resize_up", "button.pageup"}
    };
    struct TModifiedBinding {
        RuntimeControls::TString name;
        RuntimeControls::TString *begin;
        RuntimeControls::TString *end;
        RuntimeControls::TString *capacity;
    };
}

void CompactBindings::Append(void *vector, const TBinding *bindings, unsigned int count) {
    const unsigned char enabled = 1;
    for (auto index = 0U; index < count; ++index) {
        ControlsRegisterBinding(vector, bindings[index].name, &bindings[index].key, &enabled);
    }
}

void CompactBindings::BuildSharedKeyboardBindings(void *, void *vector) {
    Append(vector, sharedBindings, sizeof(sharedBindings) / sizeof(sharedBindings[0]));
}

void CompactBindings::BuildKeyboardBindings(unsigned int *vector, const unsigned char *options) {
    vector[0] = 0;
    vector[1] = 0;
    vector[2] = 0;
    BuildSharedKeyboardBindings(nullptr, vector);
    const auto layout = *reinterpret_cast<const unsigned int *>(options + 68);
    if (layout <= 3) {
        const auto *keys = layoutKeys[layout <= 1 ? 0 : layout - 1];
        const unsigned char enabled = 1;
        for (auto index = 0U; index < 4; ++index) {
            ControlsRegisterBinding(vector, layoutNames[index], keys + index, &enabled);
        }
    }

    Append(vector, commonBindings, sizeof(commonBindings) / sizeof(commonBindings[0]));
}

void CompactBindings::BuildModifiedKeyboardBindings(void *, void *vector) {
    for (const auto &binding : modifiedBindings) {
        RuntimeControls::TString chord[2];
        TModifiedBinding record;
        unsigned int temporary;
        ControlsStringConstruct(&chord[0], "button.control", &temporary);
        ControlsStringConstruct(&chord[1], binding[1], &temporary);
        ControlsStringConstruct(&record.name, binding[0], &temporary);
        record.begin = chord;
        record.end = chord + 2;
        record.capacity = chord + 2;
        ControlsAppendModifiedBinding(vector, &record);
        ControlsStringRelease(&record.name);
        ControlsStringRelease(&chord[1]);
        ControlsStringRelease(&chord[0]);
    }
}
