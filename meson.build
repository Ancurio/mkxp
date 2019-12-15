project('mkxp-z', 'cpp', 'objc', 'objcpp', version: '1.2.0', default_options: ['cpp_std=c++11', 'buildtype=release'])

minimum_macos_version = get_option('macos_min_version')

# The meson build is mostly directly copied from the old CMakeLists,
# it still needs to be cleaned up

xxd = find_program('xxd', native: true)
objfw = find_program('objfw-config', native: true)
host_system = host_machine.system()

compilers = {'cpp': meson.get_compiler('cpp'), 'objc': meson.get_compiler('objc'), 'objcpp': meson.get_compiler('objcpp')}

global_sources = []
global_dependencies = []
global_include_dirs = []
global_args = []
global_link_args = []

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

# ====================
# Main source
# ====================

# Suppress warnings
global_args += ['-Wno-non-virtual-dtor', '-Wno-reorder', '-Wno-uninitialized', '-Wno-unknown-pragmas']
if compilers['objc'].get_id() == 'clang'
    global_args += ['-Wno-undefined-var-template']
    if host_system == 'darwin' or host_system == 'windows'
        global_args += '-Wno-delete-non-abstract-non-virtual-dtor'
    endif
endif

if get_option('workdir_current')
    global_args += '-DWORKDIR_CURRENT'
endif

if get_option('independent_appimage')
    global_args += '-DINDEPENDENT_APPIMAGE'
endif

if get_option('use_fakeapi')
    global_args += '-DUSE_FAKEAPI'
endif

if not get_option('console')
    global_args += '-DNO_CONSOLE'
endif

if get_option('mk')
    global_args += '-DMARIN'
endif

# Objectify our C
global_args += run_command(objfw,'--cppflags').stdout().split()
add_project_arguments(run_command(objfw,'--objcflags').stdout().split(), language:['objc','objcpp'])
add_project_link_arguments(run_command(objfw,'--libs','--ldflags').stdout().split(), language:['objc','objcpp'])

# Make sure to use ARC
if compilers['objc'].get_id() == 'clang'
    add_project_arguments(run_command(objfw,'--arc').stdout().split(), language:['objc','objcpp'])
    if host_system != 'darwin'
        add_project_arguments('-fobjc-runtime=objfw', language:['objc','objcpp'])
    endif
endif


subdir('src')
subdir('binding')
subdir('shader')
subdir('assets')

#all_sources = [main, bindings, processed_shaders, processed_assets]
global_include_dirs += include_directories('src', 'binding')

# Disable some warnings
#add_project_arguments(['-Wno-reorder', '-Wno-non-virtual-dtor', '-Wno-uninitialized'], language: 'cpp')

if host_system == 'windows'
    subdir('windows')
    global_sources += windows_resources
    global_include_dirs += include_directories('windows')
elif host_system == 'darwin'
    subdir('macos')
    if compilers['cpp'].get_id() == 'clang'
        add_project_arguments('-stdlib=libc++', language: ['cpp','objcpp'])
        add_project_arguments('-std=c++11', language: 'objcpp')
        global_args += '-mmacosx-version-min='+minimum_macos_version
        global_link_args += '-mmacosx-version-min='+minimum_macos_version
    endif
else
    subdir('linux')
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
