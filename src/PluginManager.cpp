#include "PluginManager.h"
#include <dlfcn.h>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>

using namespace std;

bool LogFlag = false;

#define LogPrint(fmt, ...)                                                              \
    if (LogFlag) {                                                                      \
        printf("[%s:%d] %s" fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    }

class Plugin {
public:
    PluginResult Load();
    PluginResult TryUnload();
    PluginSymbol GetSymbol(const string& symbolName);
    PluginResult FreeSymbol(PluginSymbol pluginSymbol);
    PluginHandle GetHandle() const;
    const string& GetName() const;
    const string& GetPath() const;
    Plugin(const string& path, const string& name);

private:
    string Path;
    string Name;
    PluginHandle pluginHandle;
    mutable mutex Mutex;
    bool Opened;
    mutable unordered_map<PluginSymbol, int> RefCount;
};

class PluginManager {
public:
    tuple<PluginHandle, PluginResult> LoadPlugin(const string& path, const string& name);
    PluginResult UnloadPlugin(PluginHandle pluginHandle);
    tuple<shared_ptr<Plugin>, PluginResult> FindPlugin(PluginHandle pluginHandle);
    tuple<shared_ptr<Plugin>, PluginResult> FindPluginByName(const string& name);
    tuple<shared_ptr<Plugin>, PluginResult> FindPluginByPath(const string& path);

private:
    unordered_map<PluginHandle, std::shared_ptr<Plugin>> Plugins;
    mutex Mutex;
};

Plugin::Plugin(const string& path, const string& name)
    : Path(path)
    , Name(name)
    , Opened(false)
{
}

PluginResult Plugin::Load()
{
    scoped_lock Lck(Mutex);
    if (Opened) {
        LogPrint("Plugin %s is already loaded", Name.c_str());
        return PLUGIN_OK;
    }
    pluginHandle = dlopen(Path.c_str(), RTLD_LAZY);
    if (pluginHandle == nullptr) {
        LogPrint("Failed to load plugin %s(%s): %s", Name.c_str(), Path.c_str(), dlerror());
        return PLUGIN_DLOPEN_ERR;
    }

    Opened = true;

    LogPrint("Loaded plugin %s", Name.c_str());
    return PLUGIN_OK;
}

PluginResult Plugin::TryUnload()
{
    scoped_lock Lck(Mutex);
    if (!Opened) {
        LogPrint("Plugin %s is not loaded", Name.c_str());
        return PLUGIN_NOT_LOAD;
    }
    for (auto& Ref : RefCount) {
        if (Ref.second > 0) {
            LogPrint("Plugin %s is busy", Name.c_str());
            return PLUGIN_BUSY;
        }
    }

    RefCount.clear();
    dlclose(pluginHandle);
    pluginHandle = nullptr;
    Opened = false;
    LogPrint("Unloaded plugin %s", Name.c_str());
    return PLUGIN_OK;
}

PluginSymbol Plugin::GetSymbol(const string& symbolName)
{
    scoped_lock Lck(Mutex);
    if (!Opened) {
        LogPrint("Plugin %s is not loaded", Name.c_str());
        return nullptr;
    }
    PluginSymbol Symbol = dlsym(pluginHandle, symbolName.c_str());
    if (Symbol == nullptr) {
        LogPrint("Failed to get symbol %s from plugin %s: %s", symbolName.c_str(), Name.c_str(), dlerror());
        return nullptr;
    }
    RefCount[Symbol]++;
    LogPrint("Get symbol %s from plugin %s", symbolName.c_str(), Name.c_str());
    return Symbol;
}

PluginResult Plugin::FreeSymbol(PluginSymbol pluginSymbol)
{
    scoped_lock Lck(Mutex);
    if (!Opened) {
        return PLUGIN_NOT_LOAD;
    }
    if (RefCount.find(pluginSymbol) == RefCount.end()) {
        return PLUGIN_SYM_NOT_FOUND;
    }
    if (RefCount[pluginSymbol] > 0) {
        RefCount[pluginSymbol]--;
        if (RefCount[pluginSymbol] == 0) {
            RefCount.erase(pluginSymbol);
        }
    }
    return PLUGIN_OK;
}

PluginHandle Plugin::GetHandle() const
{
    scoped_lock Lck(Mutex);
    return pluginHandle;
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

tuple<PluginHandle, PluginResult> PluginManager::LoadPlugin(const string& path, const string& name)
{
    scoped_lock Lck(Mutex);
    for (auto& Plugin : Plugins) {
        if (Plugin.second->GetName() == name || Plugin.second->GetPath() == path) {
            LogPrint("Plugin %s is already loaded", name.c_str());
            return { INVALID_PLUGIN_HANDLE, PLUGIN_EXISTS };
        }
    }
    auto plugin = make_shared<Plugin>(path, name);
    PluginResult Result = plugin->Load();
    if (Result != PLUGIN_OK) {
        LogPrint("Failed to load plugin %s: %s", name.c_str(), dlerror());
        return { INVALID_PLUGIN_HANDLE, Result };
    }
    Plugins[plugin->GetHandle()] = plugin;
    LogPrint("Loaded plugin %s", name.c_str());
    return { plugin->GetHandle(), PLUGIN_OK };
}

tuple<shared_ptr<Plugin>, PluginResult> PluginManager::FindPlugin(PluginHandle pluginHandle)
{
    scoped_lock Lck(Mutex);
    auto It = Plugins.find(pluginHandle);
    if (It == Plugins.end()) {
        return { nullptr, PLUGIN_NOT_FOUND };
    }
    return { It->second, PLUGIN_OK };
}

tuple<shared_ptr<Plugin>, PluginResult> PluginManager::FindPluginByName(const string& name)
{
    scoped_lock Lck(Mutex);
    for (auto& Plugin : Plugins) {
        if (Plugin.second->GetName() == name) {
            return { Plugin.second, PLUGIN_OK };
        }
    }
    return { nullptr, PLUGIN_NOT_FOUND };
}

tuple<shared_ptr<Plugin>, PluginResult> PluginManager::FindPluginByPath(const string& path)
{
    scoped_lock Lck(Mutex);
    for (auto& Plugin : Plugins) {
        if (Plugin.second->GetPath() == path) {
            return { Plugin.second, PLUGIN_OK };
        }
    }
    return { nullptr, PLUGIN_NOT_FOUND };
}

PluginResult PluginManager::UnloadPlugin(PluginHandle pluginHandle)
{
    auto [plugin, result] = FindPlugin(pluginHandle);
    if (result != PLUGIN_OK) {
        return result;
    }
    auto ret = plugin->TryUnload();
    if (ret != PLUGIN_OK) {
        LogPrint("Failed to unload plugin %s: %s", plugin->GetName().c_str(), dlerror());
        return ret;
    }
    Plugins.erase(pluginHandle);
    LogPrint("Unloaded plugin %s", plugin->GetName().c_str());
    return PLUGIN_OK;
}

///////////////////////////////////////////

PluginManager PluginManagerInstance;

PluginHandle LoadPlugin(const char* pluginPath, const char* name)
{
    return get<0>(PluginManagerInstance.LoadPlugin(pluginPath, name));
}

PluginResult UnloadPlugin(PluginHandle pluginHandle)
{
    return PluginManagerInstance.UnloadPlugin(pluginHandle);
}

PluginSymbol GetPluginSymbol(PluginHandle pluginHandle, const char* symbolName)
{
    auto [plugin, result] = PluginManagerInstance.FindPlugin(pluginHandle);
    if (result != PLUGIN_OK) {
        return nullptr;
    }
    return plugin->GetSymbol(symbolName);
}

PluginResult FreePluginSymbol(PluginHandle pluginHandle, PluginSymbol pluginSymbol)
{
    auto [plugin, result] = PluginManagerInstance.FindPlugin(pluginHandle);
    if (result != PLUGIN_OK) {
        LogPrint("Plugin %s is not loaded", plugin->GetName().c_str());
        return result;
    }
    return plugin->FreeSymbol(pluginSymbol);
}

PluginHandle FindPluginByName(const char* name)
{
    auto [plugin, result] = (PluginManagerInstance.FindPluginByName(name));
    if (result != PLUGIN_OK) {
        return INVALID_PLUGIN_HANDLE;
    }
    return plugin->GetHandle();
}

PluginHandle FindPluginByPath(const char* path)
{
    auto [plugin, result] = (PluginManagerInstance.FindPluginByPath(path));
    if (result != PLUGIN_OK) {
        return INVALID_PLUGIN_HANDLE;
    }
    return plugin->GetHandle();
}

void SetLogFlag(bool flag)
{
    LogFlag = flag;
}
