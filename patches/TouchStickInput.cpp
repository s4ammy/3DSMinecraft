#include "TouchStick.h"

#if defined(_MSC_VER) || __STDC_HOSTED__
#include <cmath>
#endif

bool TouchStick::Update(TInputState &state, int x, int y, bool held, bool gameplay) {
    return UpdateConfigured(state, x, y, held, gameplay, TSettings{});
}

bool TouchStick::UpdateConfigured(TInputState &state, int x, int y, bool held, bool gameplay, const TSettings &settings) {
    state.lookX = 0;
    state.lookY = 0;
    const auto dx = static_cast<float>(x - settings.x);
    const auto dy = static_cast<float>(settings.y - y);
    const auto distanceSquared = dx * dx + dy * dy;
    if (held && !state.wasHeld) {
        state.ownsTouch = gameplay && settings.radius >= 8 && settings.deadZone >= 0 &&
            settings.deadZone < settings.radius - 6 && distanceSquared <= settings.radius * settings.radius;
        state.canceled = false;
    }

    const auto consumed = state.ownsTouch;
    state.wasHeld = held;
    if (!held) {
        state.ownsTouch = false;
        state.canceled = false;
        return consumed;
    }

    if (!gameplay) {
        state.canceled = true;
    }

    if (consumed && !state.canceled && distanceSquared > settings.deadZone * settings.deadZone) {
#if defined(_MSC_VER) || __STDC_HOSTED__
        const auto distance = std::sqrt(distanceSquared);
#else
        //The injected ARM payload has no standard library.
        const auto distance = __builtin_sqrtf(distanceSquared);
#endif
        const auto fullDistance = static_cast<float>(settings.radius - 6);
        const auto strength = distance < fullDistance ?
            (distance - settings.deadZone) * (100.0f / (fullDistance - settings.deadZone)) :
            100.0f;
        state.lookX = static_cast<short>(dx * strength / distance);
        state.lookY = static_cast<short>(dy * strength / distance);
    }

    return consumed;
}
