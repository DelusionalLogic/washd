washc_sources = [
	'main.c',
]

cc = meson.get_compiler('c')

washc_exe = executable(
	'washc',
	washc_sources,
	include_directories : [inc],
	dependencies: []
)
test('washc', washc_exe)
