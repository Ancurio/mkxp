project('mkxp',
	'c', 'cpp',
	default_options : [ 'c_std=c11', 'cpp_std=c++14'])
incdirs = include_directories(['assets', 'src'])

# xxd embedding 
xxd_sh = find_program('tools/xxd.sh')
xxd_exe = find_program('xxd')
sed_exe = find_program('sed')

#bin2h_exe = executable('bin2h', 'tools/bin2h.c', native : true)
xxd_gen = generator(xxd_sh,
	output : '@PLAINNAME@.xxd',
	arguments : [ xxd_exe.path(), sed_exe.path(), '@INPUT@', '@OUTPUT@']
)

embedded = xxd_gen.process(
	[
		'assets/icon.png',
		'assets/liberation.ttf',
		'shader/bitmapBlit.frag',
		'shader/blur.frag',
		'shader/blurH.vert',
		'shader/blurV.vert',
		'shader/common.h',
		'shader/flashMap.frag',
		'shader/flatColor.frag',
		'shader/gray.frag',
		'shader/hue.frag',
		'shader/minimal.vert',
		'shader/plane.frag',
		'shader/simple.frag',
		'shader/simple.vert',
		'shader/simpleAlpha.frag',
		'shader/simpleAlphaUni.frag',
		'shader/simpleColor.frag',
		'shader/simpleColor.vert',
		'shader/simpleMatrix.vert',
		'shader/sprite.frag',
		'shader/sprite.vert',
		'shader/tilemap.vert',
		'shader/tilemapvx.vert',
		'shader/trans.frag',
		'shader/transSimple.frag'
	]
)

boost = dependency('boost', modules : ['program_options'])
openal = dependency('openal')
physfs = dependency('physfs', version : '>=2.1')
pixman = dependency('pixman-1')
sdl2 = dependency('sdl2')
sdl2_image = dependency('SDL2_image')
sdl2_ttf = dependency('SDL2_ttf')
sdl_sound = dependency('SDL_sound')
sigcxx = dependency('sigc++-2.0')
vorbisfile = dependency('vorbisfile')
zlib = dependency('zlib')

fluidsynth = []
if get_option('SHARED_FLUID')
	fluidsynth = dependency('fluidsynth')
	add_global_arguments('-DSHARED_FLUID', language : 'cpp')
endif

if get_option('RGSS2')
	add_global_arguments('-DRGSS2', language : 'cpp')
endif

res = []
manifest = []
platdeps = []
if host_machine.system() == 'windows'
conf_data = configuration_data()
conf_data.set('ARCH', host_machine.cpu())
manifest = configure_file(
	input : 'assets/mkxp.manifest.in',
	output : 'mkxp.manifest',
	configuration : conf_data
)
configure_file(
	input : 'assets/runtime.manifest.in',
	output : 'runtime.manifest',
	configuration : conf_data
)
win = import('windows')
res = win.compile_resources([ 'assets/resource.rc' ],
	include_directories : incdirs)
endif

subdir('-'.join(['binding', get_option('BINDING').to_lower()]))

subdir('src')

executable(
	'mkxp',
	sources : [ embedded, main_src, res ],
	link_with : binding,
	dependencies : [boost, fluidsynth, openal, physfs, pixman, sdl2,
			sdl2_image, sdl2_ttf, sdl_sound, sigcxx, vorbisfile, zlib ],
	include_directories : [ incdirs ]
)
