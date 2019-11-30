gen_files = custom_target('simple_button_gen',
  input: 'simple_button.eflui',
  output: ['simple_button.c', 'simple_button.h'],
  command: [efl_ui_cc, '--beta-support', '@INPUT@', '@OUTPUT0@', '@OUTPUT1@']
)

executable('simple_button',
  [gen_files] + ['main.c'],
  dependencies: dependency('elementary'),
)
