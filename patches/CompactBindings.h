#pragma once

namespace CompactBindings {
    struct TBinding {
        const char *name;
        unsigned char key;
    };

    void Append(void *vector, const TBinding *bindings, unsigned int count);
    extern "C" void BuildDictationBindings(void *options, void *vector);
    extern "C" void BuildKeyboardBindings(unsigned int *vector, const unsigned char *options);
    extern "C" void BuildSharedKeyboardBindings(void *options, void *vector);
    extern "C" void BuildModifiedKeyboardBindings(void *options, void *vector);
}
