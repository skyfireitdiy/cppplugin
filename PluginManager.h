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
    };

#define INVALID_PLUGIN_HANDLE ((PluginHandle)0)

    PluginHandle LoadPlugin(const char* PluginPath, const char* Name);
    PluginResult UnloadPlugin(PluginHandle Plugin);
    PluginSymbol GetPluginSymbol(PluginHandle Plugin, const char* SymbolName);
    PluginResult FreePluginSymbol(PluginHandle Plugin, PluginSymbol Symbol);
    PluginHandle FindPluginByName(const char* Name);
    PluginHandle FindPluginByPath(const char* Path);

#ifdef __cplusplus
}
#endif
