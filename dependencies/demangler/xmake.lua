-- Copyright (C) 2024-2025  ilobilo

target("demangler")
    set_kind("static")
    set_toolchains("ilobilix-clang")
    add_deps("ilobilix.headers", "freestnd-cxx-hdrs", "freestnd-c-hdrs", "string", "smart-ptr")

    add_includedirs("demangler/include", { public = true })
    add_files(
        "demangler/source/ItaniumDemangle.cpp",
        "demangler/source/cxa_demangle.cpp"
    )