tmp = library('efl_ui_cc_format',
    ['ui_tree.c', 'Efl_Ui_Format.h', 'parser.c', 'validator.c', 'outputter.c', 'eolian_bridge.c'],
    dependencies : [dependency('eina'), dependency('eolian')]
)

efl_ui_format =  declare_dependency(
  include_directories: include_directories('.'),
  link_with: tmp,
)
