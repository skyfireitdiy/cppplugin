_Pragma("once")

#ifdef __cplusplus
    extern "C"
{
#endif

    typedef void* PluginHandle;
    typedef void* PluginSymbol;

    enum PluginResult {
        PLUGIN_OK,
        PLUGIN_DLOPEN_ERR,
        PLUGIN_NOT_LOAD,
        PLUGIN_BUSY,
        PLUGIN_SYM_NOT_FOUND,
        PLUGIN_NOT_FOUND,
        PLUGIN_EXISTS,
        PLUGIN_MODULE_INIT_ERROR,
    };

#define INVALID_PLUGIN_HANDLE ((PluginHandle)0)

#ifndef PLUGIN_MODULE
#define _GNU_SOURCE
#include <dlfcn.h>

    extern PluginHandle (*LoadPlugin)(const char* pluginPath, const char* name);
    extern PluginResult (*UnloadPlugin)(PluginHandle plugin);
    extern PluginSymbol (*GetPluginSymbol)(PluginHandle plugin, const char* symbolName);
    extern PluginResult (*FreePluginSymbol)(PluginHandle plugin, PluginSymbol pluginSymbol);
    extern PluginHandle (*FindPluginByName)(const char* name);
    extern PluginHandle (*FindPluginByPath)(const char* path);
    extern void (*SetLogFlag)(bool flag);

    static inline PluginResult PluginModuleInit()
    {

#define DLSYM_FUNC(name)                                   \
    name = (typeof(name))(dlsym(RTLD_DEFAULT, #name "_")); \
    if (name == nullptr) {                                 \
        return PLUGIN_MODULE_INIT_ERROR;                   \
    }

        DLSYM_FUNC(LoadPlugin);
        DLSYM_FUNC(UnloadPlugin);
        DLSYM_FUNC(GetPluginSymbol);
        DLSYM_FUNC(FreePluginSymbol);
        DLSYM_FUNC(FindPluginByName);
        DLSYM_FUNC(FindPluginByPath);
        DLSYM_FUNC(SetLogFlag);
        return PLUGIN_OK;

#undef DLSYM_FUNC
    }
#endif

#ifdef __cplusplus
}
#endif
