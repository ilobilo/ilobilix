package("cwalk")
    add_urls("https://github.com/likle/cwalk.git")
    add_versions("latest", "master")

    on_install(function (package)
        io.writefile("xmake.lua", [[
            target("cwalk")
                set_kind("static")
                add_languages("gnu17")

                add_includedirs("include")
                add_files("src/cwalk.c")
        ]])
        local configs = { }
        import("package.tools.xmake").install(package, configs)
        os.cp("include", package:installdir())
    end)