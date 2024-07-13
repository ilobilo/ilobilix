-- Copyright (C) 2022-2024  ilobilo

target("modules.noarch.e1000")
    add_deps("modules.dependencies")
    set_values("modules.is_external", true)

    set_kind("object")
    add_files("*.cpp")