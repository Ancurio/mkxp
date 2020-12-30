project('mkxp-z', 'c', 'cpp', version: '1.3.0', meson_version: '>=0.47.0', default_options: ['cpp_std=c++17', 'buildtype=release'])

xxd = find_program('xxd', native: true)
host_system = host_machine.system()

compilers = {'cpp': meson.get_compiler('cpp')}

global_sources = []
global_dependencies = []
global_include_dirs = []
global_args = []
global_link_args = []

sizeof = {'void*': compilers['cpp'].sizeof('void*'),
          'long': compilers['cpp'].sizeof('long')
         }
win64 = (sizeof['void*'] != sizeof['long'])

# Fill macOS version info
minimum_macos_version = ''
current_macos_version = ''
if host_system == 'darwin'
    current_macos_version = run_command('sw_vers', '-productVersion').stdout().strip()
    macosx_version_min = get_option('macos_min_version')
    if macosx_version_min != ''
        minimum_macos_version = macosx_version_min
    else
        minimum_macos_version = current_macos_version
    endif
endif

global_args += '-DMKXPZ_BUILD_MESON'
# ====================
# Ext libs
# ====================

# STEAMWORKS

steamworks = false
steamworks_path = get_option('steamworks_path')
if steamworks_path != ''
    libname = 'steam_api'
    if host_system == 'linux'
        if sizeof['void*'] == 4
            bindir = 'linux32'
        else
            bindir = 'linux64'
        endif
    elif host_system == 'windows'
        if win64 == true
            bindir = 'win64'
            libname += '64'
        else
            bindir = ''
        endif
    else
        bindir = 'osx'
    endif

    steam_libpath = steamworks_path + '/redistributable_bin/' + bindir
    steamlib = compilers['cpp'].find_library(libname, required: false, dirs: [steam_libpath])

    if steamlib.found() == true
        global_include_dirs += include_directories('steamshim')
        global_args += '-DHAVE_STEAMSHIM'
        global_sources += 'steamshim/steamshim_child.c'
        steamworks = true
    endif
endif

# BOOST UNORDERED
global_include_dirs += include_directories('boost-unordered')

# SDL2_SOUND
subdir('SDL2_sound')

# GLES
gfx_backend = get_option('gfx_backend')
if gfx_backend == 'gles'
    # Needs to be manually set up for now
    global_args += '-DGLES2_HEADER'
elif gfx_backend == 'gl'
    global_dependencies += dependency('gl')
    # boop
endif

# ====================
# Main source
# ====================

# Suppress warnings
global_args += ['-Wno-non-virtual-dtor', '-Wno-reorder', '-Wno-uninitialized', '-Wno-unknown-pragmas', '-Wno-unknown-warning-option', '-Wno-deprecated-register']
if compilers['cpp'].get_id() == 'clang'
    global_args += ['-Wno-undefined-var-template', '-Wno-delete-non-abstract-non-virtual-dtor']
endif
if host_system == 'windows'
    global_args += '-Wno-unknown-attributes'
elif host_system == 'darwin'
    global_args += '-Wno-deprecated-declarations'
endif

# Decide whether or not to use MiniFFI
miniffi = get_option('use_miniffi')
if miniffi == true
    if win64 != true
        miniffi = true
        global_args += '-DUSE_MINIFFI'
    else
        warning('64-bit MiniFFI is only supported on Linux and macOS.')
        warning('To use MiniFFI/Win32API on Windows, target 32-bit.')
        miniffi = false
    endif
endif

# Defines
if get_option('workdir_current')
    global_args += '-DWORKDIR_CURRENT'
endif

if get_option('easypoke') == true and miniffi == true
    global_args += '-DEASY_POKE'
endif

if get_option('cxx11_experimental') == true
    global_args += '-DUSE_EXPERIMENTAL_FILESYSTEM'
endif

if not get_option('console')
    global_args += '-DNO_CONSOLE'
endif

if get_option('force32') == true
    global_args += '-m32'
endif

build_static = false
if get_option('static_executable') == true and host_system == 'windows'
    build_static = true
endif

if host_system != 'darwin' or current_macos_version.version_compare('<10.15')
    global_args += '-DTHREADED_GLINIT'
endif

subdir('src')
subdir('binding')
subdir('shader')
subdir('assets')
subdir('scripts')

global_include_dirs += include_directories('src', 'binding')

rpath = ''

if host_system == 'windows'
    subdir('windows')
    global_sources += windows_resources
    global_include_dirs += include_directories('windows')
elif host_system == 'darwin'
    subdir('macos')
    rpath = '@executable_path/../libs'
    add_project_arguments('-stdlib=libc++', language: ['cpp'])
    if minimum_macos_version != ''
        add_project_arguments('-mmacosx-version-min='+minimum_macos_version, language: ['c', 'cpp'])
        add_project_link_arguments('-mmacosx-version-min='+minimum_macos_version, language: ['c', 'cpp'])
    endif
    warning('Please consider using the Xcode project located in the `macos` directory instead.')
else
    subdir('linux')
    rpath = '$ORIGIN/lib'
    if get_option('appimage') != true
        if sizeof['long'] == 8 and get_option('force32') != true
            rpath += '64'
        else
            rpath += '32'
        endif
    endif
endif

exe_name = meson.project_name()
if steamworks == true
    exe_name = meson.project_name() + '_rt'
    la = ''
    if build_static == true
        la = '-static'
    endif
    shim_args = ['-DGAME_LAUNCH_NAME="' + exe_name + '"']
    if get_option('steam_appid') != ''
        shim_args += '-DSTEAM_APPID=' + get_option('steam_appid')
    endif

    if get_option('console') == true
        shim_args += '-DSTEAMSHIM_DEBUG'
    endif

    executable(meson.project_name(),
        sources: files('steamshim/steamshim_parent.cpp'),
        dependencies: steamlib,
        include_directories: (steamworks_path + '/public'),
        cpp_args: shim_args,
        link_args: la.split(),
        gui_app: (get_option('console') == false),
        install: (host_system != 'windows'))
endif

if host_system == 'linux' and get_option('appimage') == false
    exe_name += '.' + host_machine.cpu()
endif

executable(exe_name,
    sources: global_sources,
    dependencies: global_dependencies,
    include_directories: global_include_dirs,
    install_rpath: rpath,
    link_args: global_link_args,
    cpp_args: global_args,
    objc_args: global_args,
    objcpp_args: global_args,
    gui_app: (get_option('console') == false),
    install: (host_system != 'windows')
)
