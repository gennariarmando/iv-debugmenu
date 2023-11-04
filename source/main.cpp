#include "plugin.h"
#include "menu.h"

#include "CMenuManager.h"

void DebugMenuIV() {
    plugin::Events::initEngineEvent += []() {
        DebugMenuInit();
    };
    
    plugin::Events::shutdownEngineEvent += []() {
        DebugMenuShutdown();
    };
    
    plugin::Events::gameProcessEvent += []() {
        if (CMenuManager::m_MenuActive)
            return;
        DebugMenuProcess();
    };
    
    plugin::CdeclEvent <plugin::AddressList<0xB01A95, plugin::H_CALL>,
        plugin::PRIORITY_AFTER, plugin::ArgPickNone, uint32_t()> render;

    render.SetRefAddr(plugin::GetPattern("E8 ? ? ? ? 5F 5E 5D 5B 83 C4 04 E9 ? ? ? ? CC", 0));
    render +=[]() {
        if (CMenuManager::m_MenuActive)
            return;
        DebugMenuRender();
    };
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DebugMenuIV();
    }

    return TRUE;
}
