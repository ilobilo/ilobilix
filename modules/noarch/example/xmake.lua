-- Copyright (C) 2022-2024  ilobilo

-- follow the naming scheme
target("modules.noarch.example")
    -- these lines are necessary
    set_default(false)
    add_deps("modules.dependencies")
    set_kind("shared")

    -- change this to false to link the module to kernel
    set_values("modules.is_external", true)

    add_files("*.cpp")