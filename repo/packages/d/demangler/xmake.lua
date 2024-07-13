package("demangler")
    add_urls("https://github.com/ilobilo/demangler.git")
    add_versions("latest", "master")

    add_deps("libstdcxx-headers", "string", "smart_ptr")

    on_install(function (package)
        io.writefile("xmake.lua", [[
            add_requires("libstdcxx-headers", "string", "smart_ptr")
            target("demangler")
                set_kind("static")
                add_packages("libstdcxx-headers", "string", "smart_ptr")

                add_languages("c++23")

                add_includedirs("include")
                add_files(
                    "source/ItaniumDemangle.cpp",
                    "source/cxa_demangle.cpp"
                )
        ]])
        local configs = { }
        import("package.tools.xmake").install(package, configs)
        os.cp("include", package:installdir())
    end)
