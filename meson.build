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
if cpp_compiler_id not in ['msvc', 'clang-cl']
  warning('Unsupported compiler')
endif

# Embed the UAC manifest as an RT_MANIFEST resource rather than letting the linker
# synthesise one (/MANIFEST:EMBED). lld < 22 emits the trustInfo element without its
# asm.v3 namespace, so the XML we ship would depend on the toolchain version.
manifest_rc = find_program('llvm-rc', 'rc')
manifest_res = custom_target('app-manifest',
  input: 'app.rc',
  output: 'app.res',
  command: [manifest_rc, '/I', meson.current_source_dir(), '/fo', '@OUTPUT@', '@INPUT@'])

executable('win11-toggle-rounded-corners',
           'main.cpp', manifest_res,
           cpp_args: ['/Brepro'],
           link_args: ['/DEBUG:NONE', '/Brepro', '/OPT:ICF'])
