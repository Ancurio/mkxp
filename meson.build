project('mkxp-z', 'cpp', 'objc', 'objcpp', version: '1.2.0', meson_version: '>=0.47.0', default_options: ['cpp_std=c++11', 'buildtype=release', 'warning_level=0'])

minimum_macos_version = get_option('macos_min_version')

xxd = find_program('xxd', native: true)
objfw = find_program('objfw-config', native: true)
host_system = host_machine.system()

compilers = {'cpp': meson.get_compiler('cpp'), 'objc': meson.get_compiler('objc'), 'objcpp': meson.get_compiler('objcpp')}

if compilers['objc'].get_id() != 'clang'
    error('This program must be built with Clang! ( try: OBJC=clang CXX=clang++ OBJCXX=clang++ meson build )')
endif

global_sources = []
global_dependencies = []
global_include_dirs = []
global_args = []
global_link_args = []

sizeof = {'void*': compilers['cpp'].sizeof('void*'),
          'long': compilers['cpp'].sizeof('long')
         }
win64 = (sizeof['void*'] != sizeof['long'])
# ====================
# Ext libs
# ====================

# DISCORD

discord = false
discord_libpath = get_option('discord_sdk_path')
if discord_libpath != ''
    discordlib = compilers['cpp'].find_library('discord_game_sdk', required: false, dirs: [discord_libpath+'/lib/x86', discord_libpath+'/lib/x86_64'])

    if discordlib.found() == true
        global_include_dirs += include_directories(discord_libpath+'/c')
        global_args += ['-I@0@/c'.format(discord_libpath), '-DHAVE_DISCORDSDK']
        global_dependencies += discordlib
        discord = true
    endif
endif

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
        if get_option('steam_appid') != ''
            global_include_dirs += include_directories(steamworks_path + '/public')
            global_args += ['-I@0@/public'.format(steamworks_path),
                            '-DHAVE_STEAMWORKS',
                            '-DSTEAM_APPID=' + get_option('steam_appid')]
            global_dependencies += steamlib
            steamworks = true
        else
            error('Steamworks SDK was found, but steam_appid isn\'t set!')
        endif
    endif
endif

# BOOST UNORDERED
global_include_dirs += include_directories('boost-unordered')

# ====================
# Main source
# ====================

# Suppress warnings
global_args += ['-Wno-non-virtual-dtor', '-Wno-reorder', '-Wno-uninitialized', '-Wno-unknown-pragmas', '-Wno-unknown-warning-option']
if compilers['objc'].get_id() == 'clang'
    global_args += ['-Wno-undefined-var-template', '-Wno-delete-non-abstract-non-virtual-dtor']
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

if get_option('independent_appimage')
    global_args += '-DINDEPENDENT_APPIMAGE'
endif

if get_option('use_fakeapi') == true and miniffi == true
    global_args += '-DUSE_FAKEAPI'
endif

if not get_option('console')
    global_args += '-DNO_CONSOLE'
endif

if get_option('mk')
    global_args += '-DMARIN'
endif

# This MUST be disabled if building for macOS >= 10.15
if get_option('threaded_gl_init')
    global_args += '-DTHREADED_GLINIT'
endif

# Objectify our C
global_args += run_command(objfw,'--cppflags').stdout().split()
add_project_arguments(run_command(objfw,'--objcflags').stdout().split(), language:['objc','objcpp'])
add_project_link_arguments(run_command(objfw,'--libs','--ldflags').stdout().split(), language:['objc','objcpp'])

# Make sure to use ARC
add_project_arguments(run_command(objfw,'--arc').stdout().split(), language:['objc','objcpp'])
if host_system != 'darwin'
    add_project_arguments('-fobjc-runtime=objfw', language:['objc','objcpp'])
endif

# (Fix cquery thinking ObjC headers are C++ headers in VSCode)
add_project_arguments('-ObjC', language:'objc')
add_project_arguments('-ObjC++', language:'objcpp')


subdir('src')
subdir('binding')
subdir('shader')
subdir('assets')

global_include_dirs += include_directories('src', 'binding')

if host_system == 'windows'
    subdir('windows')
    global_sources += windows_resources
    global_include_dirs += include_directories('windows')
elif host_system == 'darwin'
    subdir('macos')
    add_project_arguments('-stdlib=libc++', language: ['cpp','objcpp'])
    add_project_arguments('-std=c++11', language: 'objcpp') # Meson's cpp_std doesn't work on ObjC for some reason
    global_args += '-mmacosx-version-min='+minimum_macos_version
    global_link_args += '-mmacosx-version-min='+minimum_macos_version
else
    subdir('linux')
    add_project_arguments('-std=c++11', language: 'objcpp')
endif

executable(meson.project_name(),
    sources: global_sources,
    dependencies: global_dependencies,
    include_directories: global_include_dirs,
    link_args: global_link_args,
    cpp_args: global_args,
    objc_args: global_args,
    objcpp_args: global_args,
    gui_app: (get_option('console') == false),
    install: (host_system != 'windows')
)
