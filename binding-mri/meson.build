
mri = dependency('ruby-' + get_option('mri_version'))
binding_dependencies = [mri]

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
