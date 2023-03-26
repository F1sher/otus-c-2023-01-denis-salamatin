project('otuslog', 'c')
dep_unwind = dependency('libunwind', required : false)

if dep_unwind.found()
	executable('test_otuslog', 'otuslog.c', 'test.c',
	c_args : '-DLIBUNWIND_Y -Wall -Wextra -Wpedantic -std=c11', dependencies : dep_unwind,
	link_args : '-lunwind-x86_64 -lpthread')
else
	executable('test_otuslog', 'otuslog.c', 'test.c',
	c_args : ['-Wall', '-Wextra', '-Wpedantic', '-std=c11'],
	link_args : '-lpthread')
endif