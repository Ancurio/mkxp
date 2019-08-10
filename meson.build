project('mkxp-z', 'cpp', version: '1.0', default_options: ['cpp_std=c++11'])

xxd = find_program('xxd', native: true)
host_system = host_machine.system()
compiler = meson.get_compiler('cpp')

if get_option('workdir_current')
    add_project_arguments('-DWORKDIR_CURRENT', language: 'cpp')
endif

if get_option('fix_essentials')
    add_project_arguments('-DUSE_ESSENTIALS_FIXES', language: 'cpp')
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

linker_args = []

if host_system == 'windows'
    subdir('windows')
    all_sources += windows_resources
    include_dirs += include_directories('windows')
elif host_system == 'darwin'
    if compiler.get_id() == 'clang'
        add_project_arguments('-stdlib=libc++', language: 'cpp')
    endif
endif

executable(meson.project_name(),
    sources: all_sources,
    dependencies: [main_dependencies, binding_dependencies],
    include_directories: include_dirs,
    gui_app: (get_option('console') == false)
)
