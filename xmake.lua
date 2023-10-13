add_rules("mode.debug", "mode.release")

target("cppplugin")
set_kind("static")
add_files("src/*.cpp")

target("example")
set_kind("binary")
add_files("example/*.cpp")
add_deps("cppplugin")
add_links("dl", "gtest")

target("plugin1")
set_kind("shared")
add_files("example/plugin1/*.cpp")


target("plugin2")
set_kind("shared")
add_files("example/plugin2/*.cpp")
