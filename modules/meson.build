all_modules = { }

c_args = get_option('modules_cflags').split()
cxx_args = get_option('modules_cxxflags').split()

args = [ ]
if get_option('modules_ubsan')
    args += '-fsanitize=undefined'
endif

ld_args = [
    '-relocatable',
    '-nostdlib',
    '-static'
]

subdir(arch)
subdir('noarch')

foreach name, mod : all_modules
    external = mod[0]
    dep = mod[1]

    if external == true
        lib = static_library(name,
            include_directories : incs,
            c_args : [c_args, args],
            cpp_args : [cxx_args, args],
            link_args : ld_args,
            pic : false,
            build_by_default : true,
            dependencies : [inc_deps, dep],
            install : false
        )
        ext_mods += custom_target(name + '.ko',
            input : lib.extract_all_objects(recursive : true),
            output : name + '.ko',
            command : [c.get_linker_id(), ld_args, '@INPUT@', '-o', '@OUTPUT@'],
            build_by_default : true,
            install : false,
            depends : lib
        )
    else
        mod_deps += dep
    endif
endforeach