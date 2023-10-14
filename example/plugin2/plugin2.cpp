
#include "../../src/Plugin.h"
#include <dlfcn.h>
#include <stdio.h>

extern "C" {
int plugin2_func(const char* param);
}

int plugin2_func(const char* param)
{
    PluginModuleInit();

    printf("after init\n");

    auto plugin1 = FindPluginByName("plugin1");
    printf("plugin1: %p\n", plugin1);

    if (plugin1 == INVALID_PLUGIN_HANDLE) {
        printf("plugin1 is not loaded\n");
        return 0;
    }

    int (*pfunc)(const char*) = (int (*)(const char*))GetPluginSymbol(plugin1, "plugin1_func");
    if (pfunc == nullptr) {
        printf("plugin1_func is not found\n");
        return 1;
    }

    printf("plugin2_func: %s\n", param);

    auto ret = (*pfunc)(param) + 50;

    FreePluginSymbol(plugin1, (PluginSymbol)pfunc);

    return ret;
}
