-- Copyright (C) 2022-2024  ilobilo

-- add modules here
modules = {
    "ps2",
    "vmware"
}

target("modules.x86_64")
    set_kind("phony")

    local list = { }
    for idx, val in ipairs(modules) do
        includes(val)

        local module = "modules.x86_64." .. val
        add_deps(module)
        table.insert(list, module)
    end

    before_build(function (target)
        import("core.project.project")
        local parent = project.target("modules")
        for idx, val in ipairs(list) do
            parent:add("values", "modules.deps", val)
        end
    end)