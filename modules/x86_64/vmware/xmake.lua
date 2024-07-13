-- Copyright (C) 2022-2024  ilobilo

target("modules.x86_64.vmware")
    add_deps("modules.dependencies")
    set_values("modules.is_external", false)

    set_kind("object")
    add_files("*.cpp")