-- Copyright (C) 2022-2024  ilobilo

target("modules.noarch.nvme")
    set_default(false)
    add_deps("modules.dependencies")
    set_values("modules.is_external", false)

    set_kind("object")
    add_files("*.cpp")