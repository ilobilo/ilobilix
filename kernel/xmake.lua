-- Copyright (C) 2022-2024  ilobilo

target("ilobilix.elf")
    set_kind("binary")
    set_default(false)

    add_deps("modules")

    add_ldflags(
        "-zmax-page-size=0x1000",
        { force = true }
    )

    add_packages(
        "compiler-rt-builtins", "demangler",
        "cwalk", "printf", "uacpi",
        "libstdcxx-headers", "frigg",
        "string", "smart_ptr", "veque", "parallel_hashmap",
        "fmt", "frozen", "magic_enum",
        "limine-terminal", "limine"
    )

    set_toolchains("ilobilix-clang")
    add_files("$(projectdir)/kernel/**.cpp|arch/**.cpp")
    add_files("$(projectdir)/kernel/**.cppm|arch/**.cppm")

    if is_arch("x86_64") then
        add_cxflags(
            "-mcmodel=kernel",
            "-mno-mmx",
            "-mno-sse",
            "-mno-sse2",
            { force = true }
        )
        add_ldflags("-T" .. "$(projectdir)/kernel/linker-x86_64.ld", { force = true })

        add_files("$(projectdir)/kernel/arch/x86_64/**.cpp")
        add_files("$(projectdir)/kernel/arch/x86_64/**.S")
    elseif is_arch("aarch64") then
        add_ldflags("-T" .. "$(projectdir)/kernel/linker-aarch64.ld", { force = true })

        add_files("$(projectdir)/kernel/arch/aarch64/**.cpp")
        -- add_files("$(projectdir)/kernel/arch/aarch64/**.S")
    end