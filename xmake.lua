add_rules("mode.debug", "mode.release")

add_requires("gtest")
set_languages("c++17")

if is_mode("debug") then
    add_defines("DEBUG")
end

target("cppplugin")
set_kind("shared")
add_defines("PLUGIN_MODULE")
add_files("src/*.cpp")

target("example")
set_kind("binary")
add_files("example/*.cpp")
add_deps("cppplugin")
add_links("dl", "gtest")
add_packages("gtest")

target("plugin1")
set_kind("shared")
add_files("example/plugin1/*.cpp")


target("plugin2")
set_kind("shared")
add_files("example/plugin2/*.cpp")
