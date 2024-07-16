package("limine-terminal")
    add_urls("https://github.com/ilobilo/limine-terminal.git")
    add_versions("latest", "master")

    on_install(function (package)
        io.writefile("xmake.lua", [[
            target("limine-terminal")
                set_kind("static")

                add_includedirs("fonts", "src")
                add_files(
                    "src/flanterm/backends/fb.c",
                    "src/flanterm/flanterm.c",
                    "src/stb_image.c",
                    "src/image.c",
                    "src/term.c"
                )
        ]])
        local configs = { }
        import("package.tools.xmake").install(package, configs)
        os.cp("fonts/*", package:installdir("include"))
        os.cp("src/*|**.c", package:installdir("include"))
    end)