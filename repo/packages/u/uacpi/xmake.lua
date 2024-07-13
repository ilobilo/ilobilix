package("uacpi")
    add_urls("https://github.com/UltraOS/uACPI.git")
    add_versions("latest", "master")

    on_install(function (package)
        io.writefile("xmake.lua", [[
            target("uacpi")
                set_kind("static")
                add_languages("gnu17")

                add_includedirs("include")
                add_files("source/*.c")
        ]])
        local configs = { }
        import("package.tools.xmake").install(package, configs)
        os.cp("include", package:installdir())
    end)