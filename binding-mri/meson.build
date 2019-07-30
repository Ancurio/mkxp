
ver = get_option('mri_version')
binding_dependencies = []
if ver.version_compare('>1.8') == true
    mri = dependency('ruby-' + ver)
    binding_dependencies += mri
else
    #error('Support for legacy Ruby versions is not yet complete.')
    lib = get_option('ruby_lib')
    mri = meson.get_compiler('cpp').find_library(lib)
    binding_dependencies += mri
    add_project_arguments('-DOLD_RUBY', language: 'cpp')
endif

binding_headers = files(
    'binding-util.h',
    'binding-types.h',
    'serializable-binding.h',
    'disposable-binding.h',
    'sceneelement-binding.h',
    'viewportelement-binding.h',
    'flashable-binding.h',
)

binding_source = files(
    'binding-mri.cpp',
    'binding-util.cpp',
    'table-binding.cpp',
    'etc-binding.cpp',
    'bitmap-binding.cpp',
    'font-binding.cpp',
    'graphics-binding.cpp',
    'input-binding.cpp',
    'sprite-binding.cpp',
    'viewport-binding.cpp',
    'plane-binding.cpp',
    'window-binding.cpp',
    'tilemap-binding.cpp',
    'audio-binding.cpp',
    'module_rpg.cpp',
    'filesystem-binding.cpp',
    'windowvx-binding.cpp',
    'tilemapvx-binding.cpp',
)
