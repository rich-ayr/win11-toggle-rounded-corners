project('win11-toggle-rounded-corners', 'cpp',
  version : '1.3',
  default_options : [
    'cpp_std=c++latest',
    'buildtype=release',
    'prefer_static=true',
    'warning_level=3',
    'werror=true',
    'default_library=static',
    'b_vscrt=static_from_buildtype',
    'b_lto=true',
  ])

cpp_compiler_id = meson.get_compiler('cpp').get_id()
if cpp_compiler_id in ['msvc', 'clang-cl']
  add_global_arguments(['/wd5072', '/wd4820', '/wd4455', '/wd4668', '/wd5039'], language: 'cpp')
else
  warning('Unsupported compiler')
endif

executable('win11-toggle-rounded-corners',
           'main.cpp',
           cpp_args: ['/Brepro'],
           link_args: ['/DEBUG:NONE', '/Brepro', '/OPT:ICF'])
