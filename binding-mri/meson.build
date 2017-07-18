binding_src = [
	'audio-binding.cpp',
	'binding-mri.cpp',
	'binding-util.cpp',
	'bitmap-binding.cpp',
	'etc-binding.cpp',
	'filesystem-binding.cpp',
	'font-binding.cpp',
	'graphics-binding.cpp',
	'input-binding.cpp',
	'module_rpg.cpp',
	'plane-binding.cpp',
	'sprite-binding.cpp',
	'table-binding.cpp',
	'tilemap-binding.cpp',
	'tilemapvx-binding.cpp',
	'viewport-binding.cpp',
	'window-binding.cpp',
	'windowvx-binding.cpp'
]
ruby = dependency('-'.join(['ruby', get_option('MRIVERSION')]))
binding = build_target('binding',
	binding_src,
	dependencies : [ ruby, sdl2, sigcxx ],
	include_directories : incdirs,
	target_type : 'static_library')

