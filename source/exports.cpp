#include "menu.h"

#define INTTYPES \
	X(Int8, int8_t) \
	X(Int16, int16_t) \
	X(Int32, int32_t) \
	X(Int64, int64_t) \
	X(UInt8, uint8_t) \
	X(UInt16, uint16_t) \
	X(UInt32, uint32_t) \
	X(UInt64, uint64_t)
#define FLOATTYPES \
	X(Float32, float) \
	X(Float64, double)

extern "C" {

#define X(NAME, TYPE) \
EXPORT MenuEntry* \
DebugMenuAdd##NAME(const char *path, const char *name, TYPE *ptr, TriggerFunc triggerFunc, TYPE step, TYPE lowerBound, TYPE upperBound, const char **strings) { \
	Menu *m = findMenu(path); \
	if(m == NULL) \
		return NULL; \
	MenuEntry *e = new MenuEntry_##NAME(name, ptr, triggerFunc, step, lowerBound, upperBound, strings); \
	m->appendEntry(e); \
	return e; \
}
    INTTYPES
#undef X

#define X(NAME, TYPE) \
EXPORT MenuEntry* \
DebugMenuAdd##NAME(const char *path, const char *name, TYPE *ptr, TriggerFunc triggerFunc, TYPE step, TYPE lowerBound, TYPE upperBound) { \
	Menu *m = findMenu(path); \
	if(m == NULL) \
		return NULL; \
	MenuEntry *e = new MenuEntry_##NAME(name, ptr, triggerFunc, step, lowerBound, upperBound); \
	m->appendEntry(e); \
	return e; \
}
        FLOATTYPES
#undef X

        EXPORT MenuEntry* \
        DebugMenuAddCmd(const char* path, const char* name, TriggerFunc triggerFunc) {
        Menu* m = findMenu(path);
        if (m == NULL)
            return NULL;
        MenuEntry* e = new MenuEntry_Cmd(name, triggerFunc);
        m->appendEntry(e);
        return e;
    }

    EXPORT void
        DebugMenuEntrySetWrap(MenuEntry* e, bool wrap) {
        if (e && e->type == MENUVAR)
            ((MenuEntry_Var*)e)->wrapAround = wrap;
    }

    EXPORT void
        DebugMenuEntrySetStrings(MenuEntry* e, const char** strings) {
        if (e && e->type == MENUVAR_INT)
            ((MenuEntry_Int*)e)->setStrings(strings);
    }

    EXPORT void
        DebugMenuEntrySetAddress(MenuEntry* e, void* addr) {
        if (e && e->type == MENUVAR) {
            MenuEntry_Var* ev = (MenuEntry_Var*)e;
            // HACK - we know the variable field is at the same address
            // for all int/float classes. let's hope it stays that way.
            if (ev->vartype = MENUVAR_INT)
                ((MenuEntry_Int32*)e)->variable = (int32_t*)addr;
            else if (ev->vartype = MENUVAR_FLOAT)
                ((MenuEntry_Float32*)e)->variable = (float*)addr;
        }
    }
}
