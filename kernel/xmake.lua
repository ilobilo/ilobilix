-- Copyright (C) 2022-2024  ilobilo

-- flags shared between kernel and modules
target("ilobilix.dependencies")
    set_kind("phony")
    set_toolchains("ilobilix-clang", { public = true })

    add_packages(
        "compiler-rt-builtins", "demangler",
        "cwalk", "printf", "uacpi",
        "libstdcxx-headers", "frigg",
        "string", "smart_ptr", "veque", "parallel_hashmap",
        "fmt", "frozen", "magic_enum",
        "limine-terminal", "limine",
        { public = true }
    )

    if is_arch("x86_64") then
        add_cxflags(
            "-mno-mmx",
            "-mno-sse",
            "-mno-sse2",
            { force = true, public = true }
        )
    elseif is_arch("aarch64") then
    end

target("ilobilix.modules")
    set_default(false)
    set_kind("moduleonly")
    add_deps("ilobilix.dependencies")

    add_files("interfaces/**.cppm|interfaces/arch/**.cppm")

    -- if is_arch("x86_64") then
    --     add_files("interfaces/arch/x86_64/**.cppm")
    -- elseif is_arch("aarch64") then
    --     add_files("interfaces/arch/aarch64/**.cppm")
    -- end

    on_run(function (target)
    end)

target("ilobilix.elf")
    set_default(false)
    set_kind("binary")
    -- add_deps("modules")
    add_deps("ilobilix.modules")

    add_files("**.cpp|arch/**.cpp")

    if is_arch("x86_64") then
        add_files("arch/x86_64/**.cpp")
        add_files("arch/x86_64/**.S")
    elseif is_arch("aarch64") then
        add_files("arch/aarch64/**.cpp")
        -- add_files("arch/aarch64/**.S")
    end

    -- flags just for kernel
    add_ldflags(
        "-zmax-page-size=0x1000",
        { force = true }
    )

    if is_arch("x86_64") then
        add_cxflags(
            "-mcmodel=kernel",
            { force = true }
        )
        add_ldflags(
            "-T" .. "$(projectdir)/kernel/linker-x86_64.ld",
            { force = true }
        )
    elseif is_arch("aarch64") then
        add_ldflags(
            "-T" .. "$(projectdir)/kernel/linker-aarch64.ld",
            { force = true }
        )
    end

    on_run(function (target)
    end)