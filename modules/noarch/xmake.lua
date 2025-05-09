-- Copyright (C) 2024-2025  ilobilo

-- add modules here
modules = {
    "example"
}

target("modules.noarch")
    set_default(false)
    set_kind("phony")

    local list = { }
    for idx, val in ipairs(modules) do
        includes(val)

        local module = "modules.noarch." .. val
        add_deps(module)
        table.insert(list, module)
    end

    before_config(function (target)
        import("core.project.project")
        local parent = project.target("modules")
        for idx, val in ipairs(list) do
            parent:add("values", "modules.deps", val)
        end
    end)