
#include "../../src/PluginManager.h"
#include <dlfcn.h>
#include <stdio.h>

extern "C" {
int plugin2_func(const char* param);
}

PluginHandle (*FindPluginByName_)(const char* pluginPath) = 0;
PluginSymbol (*GetPluginSymbol_)(PluginHandle pluginHandle, const char* symbolName) = 0;
PluginResult (*FreePluginSymbol_)(PluginHandle pluginHandle, PluginSymbol pluginSymbol) = 0;

void init()
{
    PluginHandle (*FindPluginByName_)(const char* pluginPath) = (PluginHandle(*)(const char*))dlsym(RTLD_DEFAULT, "FindPluginByName");
    PluginSymbol (*GetPluginSymbol_)(PluginHandle pluginHandle, const char* symbolName) = (PluginSymbol(*)(PluginHandle pluginHandle, const char* symbolName))dlsym(RTLD_DEFAULT, "GetPluginSymbol");
    PluginResult (*FreePluginSymbol_)(PluginHandle pluginHandle, PluginSymbol pluginSymbol) = (PluginResult(*)(PluginHandle pluginHandle, PluginSymbol pluginSymbol))dlsym(RTLD_DEFAULT, "FreePluginSymbol");
}

int plugin2_func(const char* param)
{
    init();

    auto plugin1 = FindPluginByName_("plugin1");

    if (plugin1 == INVALID_PLUGIN_HANDLE) {
        printf("plugin1 is not loaded\n");
        return 0;
    }

    int (*pfunc)(const char*) = (int (*)(const char*))GetPluginSymbol_(plugin1, "plugin1_func");
    if (pfunc == nullptr) {
        printf("plugin1_func is not found\n");
        return 1;
    }

    printf("plugin2_func: %s\n", param);

    auto ret = (*pfunc)(param) + 50;

    FreePluginSymbol_(plugin1, (PluginSymbol)pfunc);

    return ret;
}
