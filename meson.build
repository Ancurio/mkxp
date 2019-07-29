project('mkxp', 'cpp', version: '1.0', meson_version: '>=0.45.1')

xxd = find_program('xxd')
binding_dir = 'binding-' + get_option('binding')
host_system = host_machine.system()

subdir('src')
subdir(binding_dir)
subdir('shader')
subdir('assets')

all_sources = [main_headers, main_source, binding_headers, binding_source, processed_shaders, processed_assets]

linker_args = []

if host_system == 'windows'
    subdir('windows')
    all_sources += windows_resources
elif host_system == 'darwin'
    if meson.get_compiler('cpp').get_id() == 'clang'
        add_project_arguments(['-std=c++11','-stdlib=libc++'], language: 'cpp')
    endif
endif

executable('mkxp.@1@.@0@'.format(host_machine.cpu(), host_system),
    sources: all_sources,
    dependencies: [main_dependencies, binding_dependencies],
    include_directories: include_directories('src', binding_dir),
    link_args: '-L/usr/local/opt/ruby/lib' # homebrew ruby pc is screwed
)
