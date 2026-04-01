project('bfc', 'cpp',
  version: '0.1.0',
  license: 'GPL-3.0-or-later',
  default_options: [
    'cpp_std=c++20',
    'warning_level=2',
    'b_ndebug=if-release',
  ],
)

llvm_dep = dependency(
  'llvm',
  version: '>= 14.0',
  required: true,
  method: 'config-tool',
)

inc = include_directories('include')

bfc_sources = files(
  'src/lexer.cpp',
  'src/parser.cpp',
  'src/optimizer.cpp',
  'src/codegen.cpp',
  'src/jit_compiler.cpp',
  'src/aot_compiler.cpp',
  'src/driver.cpp',
  'src/main.cpp',
)

executable('bfc',
  bfc_sources,
  include_directories: inc,
  dependencies: [llvm_dep],
  install: true,
)

# Library target for tests (no main)
bfc_lib = static_library('bfc_core',
  files(
    'src/lexer.cpp',
    'src/parser.cpp',
    'src/optimizer.cpp',
    'src/codegen.cpp',
    'src/jit_compiler.cpp',
    'src/aot_compiler.cpp',
    'src/driver.cpp',
  ),
  include_directories: inc,
  dependencies: [llvm_dep],
)

subdir('tests')
