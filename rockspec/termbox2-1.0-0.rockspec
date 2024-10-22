package = 'Termbox2'
version = '1.0-0'
source = {
	url = 'git://github.com/iskolbin/termbox2',
	tag = 'v1.0-0',
}
description = {
	summary = 'Lua bindings to termbox2 library',
	detailed = [[]],
	homepage = 'https://github.com/iskolbin/termbox2',
	license = 'MIT/X11',
}
dependencies = {
	'lua >= 5.1'
}
build = {
	type = 'builtin',
	modules = {
		termbox2 = {
			sources = {
				'src/ltermbox2.c',
			},
		}
	}
}
