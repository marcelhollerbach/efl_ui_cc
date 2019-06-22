gen_files = custom_target('detail_display_gen',
  input: 'detail_display.json',
  output: ['detail_display.c', 'detail_display.h'],
  command: [efl_ui_cc, '--beta-support', '@INPUT@', '@OUTPUT0@', '@OUTPUT1@']
)

executable('detail_display',
  [gen_files] + ['main.c'],
  dependencies: dependency('elementary'),
)
