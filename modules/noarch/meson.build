modules = [
    'test_noarch'
]

foreach name : modules
    external = false
    module = [ ]
    subdir(name)
    all_modules += { name : [external, module] }
endforeach