# Copyright (C) 2022-2024  ilobilo

modules = [ ]

args += [
    # '-mgeneral-regs-only',
    '-mcmodel=small'
]

foreach name : modules
    external = false
    module = [ ]
    subdir(name)
    all_modules += { name : [external, module] }
endforeach