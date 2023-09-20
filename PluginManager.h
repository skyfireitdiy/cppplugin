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
    };

#define INVALID_PLUGIN_HANDLE ((PluginHandle)0)

    PluginHandle LoadPlugin(const char* PluginPath);
    PluginResult UnloadPlugin(PluginHandle Plugin);
    PluginSymbol GetPluginSymbol(PluginHandle Plugin, const char* SymbolName);
    PluginResult FreePluginSymbol(PluginHandle Plugin, PluginSymbol Symbol);

#ifdef __cplusplus
}
#endif
