package("libstdcxx-headers")
    add_urls("https://github.com/ilobilo/libstdcxx-headers.git")
    add_versions("latest", "master")

    on_install(function (package)
        os.cp("include", package:installdir())
    end)