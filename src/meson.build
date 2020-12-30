sigcxx = dependency('sigc++-2.0', version: '>=2.10.0', static: build_static)
pixman = dependency('pixman-1', static: build_static)
physfs = dependency('physfs', version: '>=2.1', static: build_static)
vorbisfile = dependency('vorbisfile', static: build_static)
sdl2_ttf = dependency('SDL2_ttf', static: build_static)
sdl2_image = dependency('SDL2_image', static: build_static)
openal = dependency('openal', static: build_static, method: 'pkg-config')
zlib = dependency('zlib', static: build_static)

# On Windows, SDL needs to be set up manually in order to avoid
# the -mwindows flag
if host_system != 'windows'
    global_dependencies += dependency('sdl2')
else
    sdl2cfg = find_program('sdl2-config')
    global_args += run_command(sdl2cfg, '--cflags').stdout().split()
    if build_static == false
        libarg = '--libs'
    else
        libarg = '--static-libs'
    endif
    link_args = run_command(sdl2cfg, libarg).stdout().split()
    foreach arg : link_args
        if arg != '-mwindows'
            global_link_args += arg
        endif
    endforeach
endif

# If OpenSSL is present, you get HTTPS support
openssl = dependency('openssl', required: false, static: build_static)
if openssl.found() == true
    global_dependencies += openssl
    global_args += '-DMKXPZ_SSL'
else
    warning('Could not locate OpenSSL. HTTPS will be disabled.')
endif

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
    global_args += '-DAL_LIBTYPE_STATIC'
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


global_include_dirs += include_directories('.',
    'audio',
    'crypto',
    'display', 'display/gl',
    'etc',
    'filesystem', 'filesystem/ghc',
    'input',
    'system',
    'util'
)

global_dependencies += [sigcxx, openal, zlib, pixman, physfs, vorbisfile, sdl2_ttf, sdl2_image]
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
    'main.cpp',
    'config.cpp',
    'eventthread.cpp',
    'settingsmenu.cpp',
    'sharedstate.cpp',
    
    'audio/alstream.cpp',
    'audio/audio.cpp',
    'audio/audiostream.cpp',
    'audio/fluid-fun.cpp',
    'audio/midisource.cpp',
    'audio/sdlsoundsource.cpp',
    'audio/soundemitter.cpp',
    'audio/vorbissource.cpp',

    'crypto/rgssad.cpp',

    'display/autotiles.cpp',
    'display/autotilesvx.cpp',
    'display/bitmap.cpp',
    'display/font.cpp',
    'display/graphics.cpp',
    'display/plane.cpp',
    'display/sprite.cpp',
    'display/tilemap.cpp',
    'display/tilemapvx.cpp',
    'display/viewport.cpp',
    'display/window.cpp',
    'display/windowvx.cpp',

    'display/gl/gl-debug.cpp',
    'display/gl/gl-fun.cpp',
    'display/gl/gl-meta.cpp',
    'display/gl/glstate.cpp',
    'display/gl/scene.cpp',
    'display/gl/shader.cpp',
    'display/gl/texpool.cpp',
    'display/gl/tileatlas.cpp',
    'display/gl/tileatlasvx.cpp',
    'display/gl/tilequad.cpp',
    'display/gl/vertex.cpp',
    
    'etc/etc.cpp',
    'etc/table.cpp',

    'filesystem/filesystem.cpp',
    'filesystem/filesystemImpl.cpp',
    
    'input/input.cpp',
    'input/keybindings.cpp',

    'system/fake-api.cpp',
    'system/systemImpl.cpp'
)

global_sources += main_source
