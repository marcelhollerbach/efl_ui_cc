tests = ['testcase1', 'testcase2', 'testcase3', 'testcase4', 'testcase5', 'testcase6', 'testcase7', 'testcase8']

env = environment()
env.set('TEST_IN_TREE', '1')


predictor = executable('predictor',
  'test_predictor.c',
  dependencies: [dependency('elementary'), dependency('eolian'), dependency('check'), efl_ui_format]
)

test('test_predictor', predictor,
  is_parallel: true,
)

abstract_tree_controller = executable('abstract_tree_controller',
  ['test_abstract_tree_controller.c',
  join_paths(meson.source_root(),'src', 'efl_ui_editor', 'abstract_tree_controller.c')],
  include_directories : editor_inc_dir,
  dependencies: [dependency('elementary'), dependency('eolian'), dependency('check'), efl_ui_format]
)

test('test_abstract_tree_controller', abstract_tree_controller,
  is_parallel: true,
  timeout: 60,
)

diff = find_program('diff')
cp = find_program('cp')

foreach test : tests
  gen_files = custom_target(test+'_gen',
    input: test+'.eflui',
    output: [test+'.c', test+'.h'],
    command: [efl_ui_cc, '--beta-support', '@INPUT@', '@OUTPUT0@', '@OUTPUT1@']
  )

  tmp = executable(test,
    ['starter.c'] + [gen_files],
    dependencies: dependency('elementary'),
    c_args : ['-DTEST_FILE='+'"'+test+'.h"']
  )

  test(test+'_run', tmp,
    env: env,
    is_parallel: true,
    timeout: 30,
  )
  test(test+'_editor_run', editor,
    env: env,
    args : [ '--test-mode', join_paths(meson.current_source_dir(),test+'.eflui')],
    is_parallel: true,
    timeout: 30,
  )
  format_target = custom_target('reformat_'+test,
    input: test+'.eflui',
    output : test+'o.eflui',
    command: [editor, '--format', '-o', '@OUTPUT@', '@INPUT@'],
  )
  test('diff_calc_'+test, diff,
    args : [join_paths(meson.current_build_dir(),test+'o.eflui'), join_paths(meson.current_source_dir(),test+'.eflui')],
    depends : format_target,
  )
endforeach
