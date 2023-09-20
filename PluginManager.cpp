#include "PluginManager.h"
#include <dlfcn.h>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>

using namespace std;

class Plugin {
public:
    PluginResult Load();
    PluginResult TryUnload();
    PluginSymbol GetSymbol(const string& SymbolName);
    PluginResult FreeSymbol(PluginSymbol Symbol);
    PluginHandle GetHandle() const;
    Plugin(const string& Path);

private:
    string Path;
    PluginHandle Handle;
    mutable mutex Mutex;
    bool Opened;
    mutable unordered_map<PluginSymbol, int> RefCount;
};

class PluginManager {
public:
    tuple<PluginHandle, PluginResult> LoadPlugin(const string& Path);
    tuple<std::shared_ptr<Plugin>, PluginResult> FindPlugin(PluginHandle Handle);

private:
    unordered_map<PluginHandle, std::shared_ptr<Plugin>> Plugins;
    mutex Mutex;
};

Plugin::Plugin(const string& Path)
    : Path(Path)
    , Opened(false)
{
}

PluginResult Plugin::Load()
{
    scoped_lock Lck(Mutex);
    if (Opened) {
        return PLUGIN_OK;
    }
    Handle = dlopen(Path.c_str(), RTLD_LAZY);
    if (Handle == nullptr) {
        return PLUGIN_DLOPEN_ERR;
    }
    return PLUGIN_OK;
}

PluginResult Plugin::TryUnload()
{
    scoped_lock Lck(Mutex);
    if (!Opened) {
        return PLUGIN_NOT_LOAD;
    }
    for (auto& Ref : RefCount) {
        if (Ref.second > 0) {
            return PLUGIN_BUSY;
        }
    }
    RefCount.clear();
    dlclose(Handle);
    Handle = nullptr;
    Opened = false;
    return PLUGIN_OK;
}

PluginSymbol Plugin::GetSymbol(const string& SymbolName)
{
    scoped_lock Lck(Mutex);
    if (!Opened) {
        return nullptr;
    }
    PluginSymbol Symbol = dlsym(Handle, SymbolName.c_str());
    RefCount[Symbol]++;
    return Symbol;
}

PluginResult Plugin::FreeSymbol(PluginSymbol Symbol)
{
    scoped_lock Lck(Mutex);
    if (!Opened) {
        return PLUGIN_NOT_LOAD;
    }
    if (RefCount.find(Symbol) == RefCount.end()) {
        return PLUGIN_SYM_NOT_FOUND;
    }
    if (RefCount[Symbol] > 0) {
        RefCount[Symbol]--;
    }
    return PLUGIN_OK;
}

PluginHandle Plugin::GetHandle() const
{
    scoped_lock Lck(Mutex);
    return Handle;
}

tuple<PluginHandle, PluginResult> PluginManager::LoadPlugin(const string& Path)
{
    scoped_lock Lck(Mutex);
    auto plugin = make_shared<Plugin>(Path);
    PluginResult Result = plugin->Load();
    if (Result != PLUGIN_OK) {
        return { INVALID_PLUGIN_HANDLE, Result };
    }
    Plugins[plugin->GetHandle()] = plugin;
    return { plugin->GetHandle(), PLUGIN_OK };
}

tuple<shared_ptr<Plugin>, PluginResult> PluginManager::FindPlugin(PluginHandle Handle)
{
    scoped_lock Lck(Mutex);
    auto It = Plugins.find(Handle);
    if (It == Plugins.end()) {
        return { nullptr, PLUGIN_NOT_FOUND };
    }
    return { It->second, PLUGIN_OK };
}

///////////////////////////////////////////

static PluginManager PluginManagerInstance;

PluginHandle LoadPlugin(const char* PluginPath)
{
    return get<0>(PluginManagerInstance.LoadPlugin(PluginPath));
}

PluginResult UnloadPlugin(PluginHandle Plugin)
{
    auto [plugin, result] = PluginManagerInstance.FindPlugin(Plugin);
    if (result != PLUGIN_OK) {
        return result;
    }
    return plugin->TryUnload();
}

PluginSymbol GetPluginSymbol(PluginHandle Plugin, const char* SymbolName)
{
    auto [plugin, result] = PluginManagerInstance.FindPlugin(Plugin);
    if (result != PLUGIN_OK) {
        return nullptr;
    }
    return plugin->GetSymbol(SymbolName);
}

PluginResult FreePluginSymbol(PluginHandle Plugin, PluginSymbol Symbol)
{
    auto [plugin, result] = PluginManagerInstance.FindPlugin(Plugin);
    if (result != PLUGIN_OK) {
        return result;
    }
    return plugin->FreeSymbol(Symbol);
}
