# Copyright (C) 2022-2024  ilobilo

modules = [
    'ps2',
    'vmware'
]

args += [
    '-march=x86-64',
    '-msoft-float',
    '-mno-mmx',
    '-mno-sse',
    '-mno-sse2',
    '-mno-red-zone',
    '-mcmodel=large'
]

foreach name : modules
    external = false
    module = [ ]
    subdir(name)
    all_modules += { name : [external, module] }
endforeach