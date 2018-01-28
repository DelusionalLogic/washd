washd_sources = [
	'main.c',
]

cc = meson.get_compiler('c')

washd_exe = executable(
	'washd',
	washd_sources,
	include_directories : [inc],
	dependencies: []
)
test('washd', washd_exe)
