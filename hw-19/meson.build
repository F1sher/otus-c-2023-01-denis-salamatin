project('hw-19', 'c')
glibdep = dependency('glib-2.0')
executable('daemon-filesize', 'daemon-filesize.c',
			      dependencies : glibdep,
			      c_args : ['-Wextra', '-Wpedantic', '-std=c11'] )
