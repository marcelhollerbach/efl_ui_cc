tests = ['testcase1', 'testcase2', 'testcase3', 'testcase4', 'testcase5', 'testcase6', 'testcase7']

env = environment()
env.set('TEST_IN_TREE', '1')

foreach test : tests
  gen_files = custom_target(test+'_gen',
    input: test+'.json',
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
endforeach
