sigcxx = dependency('sigc++-2.0', version: '>=2.10.0', static: build_static)
pixman = dependency('pixman-1', static: build_static)
physfs = dependency('physfs', version: '>=2.1', static: build_static)
vorbisfile = dependency('vorbisfile', static: build_static)
sdl2 = dependency('sdl2', static: build_static)
sdl2_ttf = dependency('SDL2_ttf', static: build_static)
sdl2_image = dependency('SDL2_image', static: build_static)
sdl_sound = dependency('SDL_sound', static: build_static)
openal = dependency('openal', static: build_static)
zlib = dependency('zlib', static: build_static)


# Windows needs to be treated like a special needs child here
explicit_libs = ''
if host_system == 'windows'
    # Newer versions of Ruby will refuse to link without these
    explicit_libs += 'libmsvcrt;libgcc;libmingwex;libgmp;'
endif
if build_static == true
    explicit_libs += 'libfreetype;libbz2;libharfbuzz;libgraphite2;'
    explicit_libs += 'libjpeg;libpng;libtiff;libwebp;liblzma;libzstd;'
    global_link_args += '-static'
endif

foreach l : explicit_libs.split(';')
        if l != ''
            global_link_args += '-l:' + l + '.a'
        endif
endforeach

alcdev_struct = 'ALCdevice_struct'
if openal.type_name() == 'pkgconfig'
    if openal.version().version_compare('>=1.20.1')
        alcdev_struct = 'ALCdevice'
    endif
endif

global_args += '-DALCDEVICE_STRUCT=' + alcdev_struct

global_include_dirs += include_directories('.')
global_dependencies += [sigcxx, openal, zlib, pixman, physfs, vorbisfile, sdl2, sdl2_ttf, sdl2_image, sdl_sound]
if host_system == 'darwin'
    global_dependencies += compilers['cpp'].find_library('iconv')
    global_dependencies += dependency('Foundation')

    if openal.type_name() != 'pkgconfig'
        add_project_arguments('-DUSE_MAC_OPENAL', language: 'cpp')
    endif
elif host_system == 'windows'
    global_dependencies += compilers['cpp'].find_library('wsock32')
endif

if get_option('shared_fluid') == true
    fluidsynth = dependency('fluidsynth', static: build_static)
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
    'main.mm',
    'audio.cpp',
    'bitmap.cpp',
    'eventthread.cpp',
    'filesystem.mm',
    'font.cpp',
    'input.cpp',
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
    'config.mm',
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
    'vorbissource.cpp',
    'windowvx.cpp',
    'tilemapvx.cpp',
    'tileatlasvx.cpp',
    'autotilesvx.cpp',
    'midisource.cpp',
    'fluid-fun.cpp',
    'lang-fun.mm',
    'rgssad.cpp'
)

if get_option('easypoke') == true and miniffi == true
    main_source += files('fake-api.mm')
endif

global_sources += main_source
