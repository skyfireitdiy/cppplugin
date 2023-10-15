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

    PluginHandle LoadPlugin(const char* pluginPath, const char* name);
    PluginResult UnloadPlugin(PluginHandle plugin);
    PluginSymbol GetPluginSymbol(PluginHandle plugin, const char* symbolName);
    PluginResult FreePluginSymbol(PluginHandle plugin, PluginSymbol pluginSymbol);
    PluginHandle FindPluginByName(const char* name);
    PluginHandle FindPluginByPath(const char* path);
    void SetLogFlag(bool flag);

#ifdef __cplusplus
}
#endif
