-- Copyright (C) 2022-2024  ilobilo

-- follow the naming scheme
target("modules.noarch.example")
    -- change this to false to link the odule to kernelm
    set_values("is_external", true)

    add_files("*.cpp")