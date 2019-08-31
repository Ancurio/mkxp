project('mkxp-z', 'cpp', 'c', version: '1.0', default_options: ['cpp_std=c++11'])

minimum_macos_version = '10.10'

# The meson build is mostly directly copied from the old CMakeLists,
# it still needs to be cleaned up

xxd = find_program('xxd', native: true)
host_system = host_machine.system()
compiler = meson.get_compiler('cpp')

if get_option('workdir_current')
    add_project_arguments('-DWORKDIR_CURRENT', language: 'cpp')
endif

if get_option('use_fakeapi')
    add_project_arguments('-DUSE_FAKEAPI', language: 'cpp')
endif

if not get_option('console')
    add_project_arguments('-DNO_CONSOLE', language: 'cpp')
endif

subdir('src')
subdir('binding')
subdir('shader')
subdir('assets')

all_sources = [main, bindings, processed_shaders, processed_assets]
include_dirs = [include_directories('src', 'binding')]

# Use cwalk
all_sources += files('cwalk/src/cwalk.c')
include_dirs += include_directories('cwalk/include')

linker_args = []

if host_system == 'windows'
    subdir('windows')
    all_sources += windows_resources
    include_dirs += include_directories('windows')
elif host_system == 'darwin'
    subdir('macos')
    if compiler.get_id() == 'clang'
        add_project_arguments('-stdlib=libc++', language: 'cpp')
    add_project_arguments('-mmacosx-version-min='+minimum_macos_version, language: 'cpp')
    add_project_arguments('-mmacosx-version-min='+minimum_macos_version, language: 'c')
    add_project_link_arguments('-mmacosx-version-min='+minimum_macos_version, language: 'cpp')
    endif
endif

executable(meson.project_name(),
    sources: all_sources,
    dependencies: [main_dependencies, binding_dependencies],
    include_directories: include_dirs,
    gui_app: (get_option('console') == false),
    install: (host_system == 'darwin')
)
