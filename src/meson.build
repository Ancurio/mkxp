sigcxx = dependency('sigc++-2.0')
pixman = dependency('pixman-1')
physfs = dependency('physfs', version: '>=2.1')
vorbisfile = dependency('vorbisfile')
sdl2 = dependency('sdl2')
sdl2_ttf = dependency('SDL2_ttf')
sdl2_image = dependency('SDL2_image')
sdl_sound = dependency('SDL_sound')
opengl = dependency('GL')

boost = dependency('boost', version: '>=1.49', modules: 'program_options')
openal = dependency('openal')
zlib = dependency('zlib')

global_include_dirs += include_directories('.')
global_dependencies += [sigcxx, openal, opengl, boost, zlib, pixman, physfs, vorbisfile, sdl2, sdl2_ttf, sdl2_image, sdl_sound]
if host_system == 'darwin'
    global_dependencies += compilers['cpp'].find_library('iconv')
    global_dependencies += dependency('CoreFoundation')

    if openal.type_name() != 'pkgconfig'
    add_project_arguments('-DUSE_MAC_OPENAL', language: 'cpp')
    endif
elif host_system == 'windows'
    global_dependencies += compiler.find_library('wsock32')
endif

if get_option('shared_fluid') == true
    fluidsynth = dependency('fluidsynth')
    add_project_arguments('-DSHARED_FLUID', language: 'cpp')
    global_dependencies += fluidsynth
endif

if get_option('default_framerate') == true
    add_project_arguments('-DDEFAULT_FRAMERATE', language: 'cpp')
endif

if get_option('cjk_fallback_font') == true
    add_project_arguments('-DCJK_FALLBACK', language: 'cpp')
endif

main_source = files(
    'main.cpp',
    'audio.cpp',
    'bitmap.cpp',
    'eventthread.cpp',
    'filesystem.mm',
    'font.cpp',
    'input.cpp',
    'iniconfig.cpp',
    'plane.cpp',
    'scene.cpp',
    'sprite.cpp',
    'table.cpp',
    'tilequad.cpp',
    'viewport.cpp',
    'window.cpp',
    'texpool.cpp',
    'shader.cpp',
    'glstate.cpp',
    'tilemap.cpp',
    'autotiles.cpp',
    'graphics.cpp',
    'gl-debug.cpp',
    'etc.cpp',
    'config.cpp',
    'settingsmenu.cpp',
    'keybindings.cpp',
    'tileatlas.cpp',
    'sharedstate.cpp',
    'gl-fun.cpp',
    'gl-meta.cpp',
    'vertex.cpp',
    'soundemitter.cpp',
    'sdlsoundsource.cpp',
    'alstream.cpp',
    'audiostream.cpp',
    'rgssad.cpp',
    'vorbissource.cpp',
    'windowvx.cpp',
    'tilemapvx.cpp',
    'tileatlasvx.cpp',
    'autotilesvx.cpp',
    'midisource.cpp',
    'fluid-fun.cpp',
    'lang-fun.cpp'
)

if get_option('use_fakeapi') == true
    main_source += files('fake-api.cpp')
endif

if discord == true
    main_source += files('discordstate.cpp')
endif

global_sources += main_source