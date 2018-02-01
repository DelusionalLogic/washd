subdir('include')
subdir('gdbus')

shared_sources = [
	gdbus_src,
	'logger.c',
	'vector.c',
	'myerror.c',
	'net/serialize.c',
]

# Hack to allow the generated .h file to be seen
hack_inc = [
	include_directories('.')
]

cc = meson.get_compiler('c')

gio_unix_dep = dependency('gio-unix-2.0')
gio_dep = dependency('gio-2.0')

static_lib = static_library(
	'shared',
	shared_sources,
	include_directories : [shared_inc],
	dependencies: [thread_dep, gio_dep, gio_unix_dep]
)

shared_dep = declare_dependency(
	include_directories: [shared_inc, hack_inc],
	link_with: static_lib,
	dependencies: [thread_dep, gio_dep, gio_unix_dep]
)
