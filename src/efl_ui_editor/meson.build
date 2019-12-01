cc = meson.get_compiler('c')

ffi = cc.find_library('ffi')
dl = cc.find_library('dl')

executable('efl_ui_editor',
  ['main.c', 'json_generator.c', 'object_generator.c'],
  dependencies: [dependency('eolian'), dependency('efl-ui'), efl_ui_format, dependency('libffi'), dl],
)
