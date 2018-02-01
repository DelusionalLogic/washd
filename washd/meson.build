washd_sources = [
	'main.c',
	'parser.c',
]

cc = meson.get_compiler('c')

myhtml_dep = dependency('myhtml')
curl_dep = dependency('libcurl')

washd_exe = executable(
	'washd',
	washd_sources,
	include_directories : [],
	dependencies: [shared_dep, myhtml_dep, curl_dep]
)
test('washd', washd_exe)
