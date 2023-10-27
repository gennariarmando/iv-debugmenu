#pragma once

extern "C" {
    struct MenuEntry;

    typedef void (*TriggerFunc)(void);
    EXPORT MenuEntry* DebugMenuAddInt8(const char* path, const char* name, int8_t* ptr, TriggerFunc triggerFunc, int8_t step, int8_t lowerBound, int8_t upperBound, const char** strings);
    EXPORT MenuEntry* DebugMenuAddInt16(const char* path, const char* name, int16_t* ptr, TriggerFunc triggerFunc, int16_t step, int16_t lowerBound, int16_t upperBound, const char** strings);
    EXPORT MenuEntry* DebugMenuAddInt32(const char* path, const char* name, int32_t* ptr, TriggerFunc triggerFunc, int32_t step, int32_t lowerBound, int32_t upperBound, const char** strings);
    EXPORT MenuEntry* DebugMenuAddInt64(const char* path, const char* name, int64* ptr, TriggerFunc triggerFunc, int64 step, int64 lowerBound, int64 upperBound, const char** strings);
    EXPORT MenuEntry* DebugMenuAddUInt8(const char* path, const char* name, uint8_t* ptr, TriggerFunc triggerFunc, uint8_t step, uint8_t lowerBound, uint8_t upperBound, const char** strings);
    EXPORT MenuEntry* DebugMenuAddUInt16(const char* path, const char* name, uint16_t* ptr, TriggerFunc triggerFunc, uint16_t step, uint16_t lowerBound, uint16_t upperBound, const char** strings);
    EXPORT MenuEntry* DebugMenuAddUInt32(const char* path, const char* name, uint32_t* ptr, TriggerFunc triggerFunc, uint32_t step, uint32_t lowerBound, uint32_t upperBound, const char** strings);
    EXPORT MenuEntry* DebugMenuAddUInt64(const char* path, const char* name, uint64* ptr, TriggerFunc triggerFunc, uint64 step, uint64 lowerBound, uint64 upperBound, const char** strings);
    EXPORT MenuEntry* DebugMenuAddFloat32(const char* path, const char* name, float* ptr, TriggerFunc triggerFunc, float step, float lowerBound, float upperBound);
    EXPORT MenuEntry* DebugMenuAddFloat64(const char* path, const char* name, double* ptr, TriggerFunc triggerFunc, double step, double lowerBound, double upperBound);
    EXPORT MenuEntry* DebugMenuAddCmd(const char* path, const char* name, TriggerFunc triggerFunc);
    EXPORT void DebugMenuEntrySetWrap(MenuEntry* e, bool wrap);
    EXPORT void DebugMenuEntrySetStrings(MenuEntry* e, const char** strings);
    EXPORT void DebugMenuEntrySetAddress(MenuEntry* e, void* addr);
}
