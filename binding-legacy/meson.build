error('Legacy bindings have not been completed yet.')

mri = meson.get_compiler('cpp').find_library('ruby')
binding_dependencies = [mri]

add_project_arguments('-DBINDING_LEGACY', language: 'cpp')

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
