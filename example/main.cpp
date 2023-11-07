#include "../src/Plugin.h"
#include <cstdlib>
#include <gtest/gtest.h>
#include <iostream>

using namespace std;

#define SO_PATH "./"


int main(int argc, char** argv)
{
    SetLogFlag(true);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(Plugin, Load)
{
    auto plugin1 = LoadPlugin(SO_PATH "libplugin1.so", "plugin1");
    EXPECT_NE(plugin1, INVALID_PLUGIN_HANDLE);
    EXPECT_EQ(UnloadPlugin(plugin1), PLUGIN_OK);
}

TEST(Plugin, LoadNotExist)
{
    auto plugin1 = LoadPlugin("so_not_exist.so", "plugin2");
    EXPECT_EQ(plugin1, INVALID_PLUGIN_HANDLE);
}

TEST(Plugin, CallFunc)
{
    auto plugin1 = LoadPlugin(SO_PATH "libplugin1.so", "plugin1");
    EXPECT_NE(plugin1, INVALID_PLUGIN_HANDLE);
    int (*pfunc)(const char*) = (int (*)(const char*))GetPluginSymbol(plugin1, "plugin1_func");
    EXPECT_NE(pfunc, nullptr);
    EXPECT_EQ(pfunc("hello world"), 50);
    EXPECT_EQ(FreePluginSymbol(plugin1, (PluginSymbol)pfunc), PLUGIN_OK);
    EXPECT_EQ(UnloadPlugin(plugin1), PLUGIN_OK);
}

TEST(Plugin, LoadTwice)
{
    auto plugin1 = LoadPlugin(SO_PATH "libplugin1.so", "plugin1");
    EXPECT_NE(plugin1, INVALID_PLUGIN_HANDLE);
    auto plugin2 = LoadPlugin(SO_PATH "libplugin1.so", "plugin1");
    EXPECT_EQ(plugin2, INVALID_PLUGIN_HANDLE);
    EXPECT_EQ(UnloadPlugin(plugin1), PLUGIN_OK);
}

TEST(Plugin, GetSymbolTwice)
{
    auto plugin1 = LoadPlugin(SO_PATH "libplugin1.so", "plugin1");
    EXPECT_NE(plugin1, INVALID_PLUGIN_HANDLE);
    int (*pfunc)(const char*) = (int (*)(const char*))GetPluginSymbol(plugin1, "plugin1_func");
    EXPECT_NE(pfunc, nullptr);
    pfunc = (int (*)(const char*))GetPluginSymbol(plugin1, "plugin1_func");
    EXPECT_NE(pfunc, nullptr);
    EXPECT_EQ(FreePluginSymbol(plugin1, (PluginSymbol)pfunc), PLUGIN_OK);
    EXPECT_NE(UnloadPlugin(plugin1), PLUGIN_OK);
    EXPECT_EQ(FreePluginSymbol(plugin1, (PluginSymbol)pfunc), PLUGIN_OK);
    EXPECT_EQ(UnloadPlugin(plugin1), PLUGIN_OK);
}

TEST(Plugin, FreeSymbolTwice)
{
    auto plugin1 = LoadPlugin(SO_PATH "libplugin1.so", "plugin1");
    EXPECT_NE(plugin1, INVALID_PLUGIN_HANDLE);
    int (*pfunc)(const char*) = (int (*)(const char*))GetPluginSymbol(plugin1, "plugin1_func");
    EXPECT_NE(pfunc, nullptr);
    EXPECT_EQ(FreePluginSymbol(plugin1, (PluginSymbol)pfunc), PLUGIN_OK);
    EXPECT_NE(FreePluginSymbol(plugin1, (PluginSymbol)pfunc), PLUGIN_OK);
    EXPECT_EQ(UnloadPlugin(plugin1), PLUGIN_OK);
}

TEST(Plugin, FindPluginByName)
{
    auto plugin1 = LoadPlugin(SO_PATH "libplugin1.so", "plugin1");
    EXPECT_NE(plugin1, INVALID_PLUGIN_HANDLE);
    auto plugin2 = FindPluginByName("plugin1");
    EXPECT_NE(plugin2, INVALID_PLUGIN_HANDLE);
    EXPECT_EQ(UnloadPlugin(plugin1), PLUGIN_OK);
}

TEST(Plugin, FindPluginByNameNotExist)
{
    auto plugin1 = FindPluginByName("plugin2");
    EXPECT_EQ(plugin1, INVALID_PLUGIN_HANDLE);
}

TEST(Plugin, FindPluginByPath)
{
    auto plugin1 = LoadPlugin(SO_PATH "libplugin1.so", "plugin1");
    EXPECT_NE(plugin1, INVALID_PLUGIN_HANDLE);
    auto plugin2 = FindPluginByPath(SO_PATH "libplugin1.so");
    EXPECT_NE(plugin2, INVALID_PLUGIN_HANDLE);
    EXPECT_EQ(UnloadPlugin(plugin1), PLUGIN_OK);
}

TEST(Plugin, FindPluginByPathNotExist)
{
    auto plugin1 = FindPluginByPath(SO_PATH "libplugin2.so");
    EXPECT_EQ(plugin1, INVALID_PLUGIN_HANDLE);
}

TEST(Plugin, LoadPluginDeps)
{
    auto plugin1 = LoadPlugin(SO_PATH "libplugin1.so", "plugin1");
    EXPECT_NE(plugin1, INVALID_PLUGIN_HANDLE);
    auto plugin2 = LoadPlugin(SO_PATH "libplugin2.so", "plugin2");
    EXPECT_NE(plugin2, INVALID_PLUGIN_HANDLE);
    int (*pfunc)(const char*) = (int (*)(const char*))GetPluginSymbol(plugin2, "plugin2_func");
    EXPECT_NE(pfunc, nullptr);
    EXPECT_EQ(pfunc("hello world"), 100);
    EXPECT_EQ(FreePluginSymbol(plugin2, (PluginSymbol)pfunc), PLUGIN_OK);
    EXPECT_EQ(UnloadPlugin(plugin1), PLUGIN_OK);
    EXPECT_EQ(UnloadPlugin(plugin2), PLUGIN_OK);
}
