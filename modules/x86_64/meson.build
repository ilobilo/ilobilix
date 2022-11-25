modules = [
    'test_x64'
]

args += [
    '-march=x86-64',
    '-mabi=sysv',
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