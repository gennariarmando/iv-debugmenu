#include "plugin.h"
#include "menu.h"

void DebugMenuIV() {
    plugin::Events::initEngineEvent += []() {
        DebugMenuInit();
    };
    
    plugin::Events::shutdownEngineEvent += []() {
        DebugMenuShutdown();
    };
    
    plugin::Events::gameProcessEvent += []() {
        DebugMenuProcess();
    };
    
    plugin::Events::drawingEvent += []() {
        DebugMenuRender();
    };
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DebugMenuIV();
    }

    return TRUE;
}
