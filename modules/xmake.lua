-- Copyright (C) 2022-2024  ilobilo

target("modules.is_external")
    set_kind("phony")

target("modules.dependencies")
    set_kind("phony")
    set_toolchains("ilobilix-clang", { public = true })

    if is_arch("x86_64") then
        add_cxflags(
            "-mcmodel=large",
            "-mno-mmx",
            "-mno-sse",
            "-mno-sse2",
            { force = true, public = true }
        )
    elseif is_arch("aarch64") then
    end

    add_packages(
        "compiler-rt-builtins", "demangler",
        "cwalk", "printf", "uacpi",
        "libstdcxx", "frigg",
        "string", "smart_ptr", "veque", "parallel_hashmap",
        "fmt", "frozen", "magic_enum",
        "limine-terminal", "limine",
        { public = true }
    )

target("modules.relocatable")
    set_kind("phony")
    add_ldflags(
        "-nostdlib",
        "-static",
        "-znoexecstack",
        "-relocatable",
        { force = true, public = true }
    )

target("modules")
    set_kind("phony")

    set_values("modules.deps", { })
    set_values("modules.external_modules", { })

    if is_arch("x86_64") then
        includes("x86_64")
        add_deps("modules.x86_64")
    elseif is_arch("aarch64") then
        includes("aarch64")
        add_deps("modules.aarch64")
    end

    includes("noarch")
    add_deps("modules.noarch")

    on_build(function (target)
        import("core.project.project")
        import("core.project.depend")
        import("core.tool.linker")

        local kernel = project.target("ilobilix.elf")

        for idx, val in ipairs(target:values("modules.deps")) do
            split = val:trim():split(".", { plain = true })
            if #split > 2 then
                local child = project.target(val)

                if not (child:get("kind") == "object") then
                    raise("please use set_kind(\"object\") for modules")
                end

                local objects = child:objectfiles()

                local values = child:get("values", "modules.is_external")
                if not (values == nil) and values["modules.is_external"] then
                    -- TODO: clean this up
                    table.remove(split, 1)
                    table.remove(split, 1)
                    local dot_ko = table.concat(split, ".") .. ".ko"
                    local out = path.join(child:targetdir(), dot_ko)

                    depend.on_changed(function ()
                        print(" => external module: " .. val .. " => " .. dot_ko)

                        os.mkdir(path.directory(out))

                        local program, argv = linker.linkargv("binary", "cc", objects, out, { target = project.target("modules.relocatable") })
                        table.insert(argv, "-relocatable")
                        os.execv(program, argv)
                    end, { files = objects })

                    for odx, obj in ipairs(objects) do
                        target:add("values", "modules.external_modules", out)
                    end
                else
                    depend.on_changed(function ()
                        print(" => internal module: " .. val)
                    end, { files = objects })

                    for odx, obj in ipairs(objects) do
                        table.insert(kernel:objectfiles(), obj)
                    end
                end
            end
        end
    end)