# Copyright (C) 2022-2023  ilobilo

modules = [
    'nvme',
    'e1000',
    'rtl8139'
]

foreach name : modules
    external = false
    module = [ ]
    subdir(name)
    all_modules += { name : [external, module] }
endforeach