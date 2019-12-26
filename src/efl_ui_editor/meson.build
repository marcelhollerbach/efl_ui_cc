cc = meson.get_compiler('c')

ffi = cc.find_library('ffi')
dl = cc.find_library('dl')

ui_files = [
  'base_ui',
  'abstract_node_ui',
  'property_item_ui',
  'property_name_ui',
  'new_property_item_ui',
  'new_children_item_ui',
  'children_item_ui',
  'value_change_bool_ui',
  'value_change_range_ui',
  'value_change_text_ui',
  'predictor_ui'
]
gen_files = []

foreach ui_file : ui_files
  gen_files += custom_target(ui_file+'_gen',
    input: ui_file+'.eflui',
    output: [ui_file+'.c', ui_file+'.h'],
    command: [efl_ui_cc, '--beta-support', '@INPUT@', '@OUTPUT0@', '@OUTPUT1@']
  )
endforeach

editor = executable('efl_ui_editor',
  ['main.c', 'json_generator.c', 'object_generator.c',
   'base_ui.c', 'display_ui.c', 'abstract_tree_controller.c',
   'predictor_ui.c', 'value_change_ui.c', gen_files],
  dependencies: [dependency('eolian'), dependency('efl-ui'), efl_ui_format, dependency('libffi'), dl],
  install: true,
)

editor_inc_dir = include_directories('.')
