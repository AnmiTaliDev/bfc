test_inc = [include_directories('../include')]
test_link = [bfc_lib]
test_deps = [llvm_dep]

foreach t : ['lexer', 'parser', 'optimizer']
  exe = executable('test_' + t,
    'test_' + t + '.cpp',
    include_directories: test_inc,
    link_with: test_link,
    dependencies: test_deps,
  )
  test(t, exe)
endforeach
