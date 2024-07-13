package("fmt")
    add_urls("https://github.com/fmtlib/fmt.git")
    add_versions("latest", "master")

    add_deps("libstdcxx-headers", "smart_ptr", "string", "veque")

    on_install(function (package)
        io.writefile("xmake.lua", [[
            add_requires("libstdcxx-headers", "smart_ptr", "string", "veque")
            target("fmt")
                set_kind("static")
                add_packages("libstdcxx-headers", "smart_ptr", "string", "veque")

                add_languages("c++23")

                add_includedirs("include")
                add_files("src/format.cc")
        ]])
        local configs = { }
        import("package.tools.xmake").install(package, configs)
        os.cp("include", package:installdir())
    end)