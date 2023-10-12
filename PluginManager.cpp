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
    const string& GetName() const;
    const string& GetPath() const;
    Plugin(const string& Path, const string& Name);

private:
    string Path;
    string Name;
    PluginHandle Handle;
    mutable mutex Mutex;
    bool Opened;
    mutable unordered_map<PluginSymbol, int> RefCount;
};

class PluginManager {
public:
    tuple<PluginHandle, PluginResult> LoadPlugin(const string& Path, const string& Name);
    tuple<std::shared_ptr<Plugin>, PluginResult> FindPlugin(PluginHandle Handle);
    tuple<std::shared_ptr<Plugin>, PluginResult> FindPluginByName(const string& Name);
    tuple<std::shared_ptr<Plugin>, PluginResult> FindPluginByPath(const string& Path);

private:
    unordered_map<PluginHandle, std::shared_ptr<Plugin>> Plugins;
    mutex Mutex;
};

Plugin::Plugin(const string& Path, const string& Name)
    : Path(Path)
    , Name(Name)
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

    Opened = true;

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

const string& Plugin::GetName() const
{
    scoped_lock Lck(Mutex);
    return Name;
}

const string& Plugin::GetPath() const
{
    scoped_lock Lck(Mutex);
    return Path;
}

tuple<PluginHandle, PluginResult> PluginManager::LoadPlugin(const string& Path, const string& Name)
{
    scoped_lock Lck(Mutex);
    for (auto& Plugin : Plugins) {
        if (Plugin.second->GetName() == Name || Plugin.second->GetPath() == Path) {
            return { INVALID_PLUGIN_HANDLE, PLUGIN_EXISTS };
        }
    }
    auto plugin = make_shared<Plugin>(Path, Name);
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

tuple<shared_ptr<Plugin>, PluginResult> PluginManager::FindPluginByName(const string& Name)
{
    scoped_lock Lck(Mutex);
    for (auto& Plugin : Plugins) {
        if (Plugin.second->GetName() == Name) {
            return { Plugin.second, PLUGIN_OK };
        }
    }
    return { nullptr, PLUGIN_NOT_FOUND };
}

tuple<shared_ptr<Plugin>, PluginResult> PluginManager::FindPluginByPath(const string& Path)
{
    scoped_lock Lck(Mutex);
    for (auto& Plugin : Plugins) {
        if (Plugin.second->GetPath() == Path) {
            return { Plugin.second, PLUGIN_OK };
        }
    }
    return { nullptr, PLUGIN_NOT_FOUND };
}

///////////////////////////////////////////

static PluginManager PluginManagerInstance;

PluginHandle LoadPlugin(const char* PluginPath, const char* Name)
{
    return get<0>(PluginManagerInstance.LoadPlugin(PluginPath, Name));
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

PluginHandle FindPluginByName(const char* Name)
{
    auto [plugin, result] = (PluginManagerInstance.FindPluginByName(Name));
    if (result != PLUGIN_OK) {
        return INVALID_PLUGIN_HANDLE;
    }
    return plugin->GetHandle();
}

PluginHandle FindPluginByPath(const char* Path)
{
    auto [plugin, result] = (PluginManagerInstance.FindPluginByPath(Path));
    if (result != PLUGIN_OK) {
        return INVALID_PLUGIN_HANDLE;
    }
    return plugin->GetHandle();
}
