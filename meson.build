project('mkxpz', 'cpp', version: '1.0')

xxd = find_program('xxd')
binding_dir = 'binding-' + get_option('binding')
host_system = host_machine.system()

if get_option('workdir_current') == true
    add_project_arguments('-DWORKDIR_CURRENT', language: 'cpp')
endif

add_project_arguments('-DBINDING_' + get_option('binding').to_upper(), language: 'cpp')

subdir('src')
subdir(binding_dir)
subdir('shader')
subdir('assets')

all_sources = [main, bindings, processed_shaders, processed_assets]
include_dirs = [include_directories('src', binding_dir)]

linker_args = []

if host_system == 'windows'
    subdir('windows')
    all_sources += windows_resources
    include_dirs += include_directories('windows')
elif host_system == 'darwin'
    if meson.get_compiler('cpp').get_id() == 'clang'
        add_project_arguments(['-std=c++11','-stdlib=libc++'], language: 'cpp')
    endif
endif

executable('@0@-@1@_@2@'.format(meson.project_name(), host_system, host_machine.cpu_family()),
    sources: all_sources,
    dependencies: [main_dependencies, binding_dependencies],
    include_directories: include_dirs
)
