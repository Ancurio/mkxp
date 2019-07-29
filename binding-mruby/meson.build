# mruby support in cmake wasn't finished, I assume it wouldn't be
# here either -- it can be done later

warning('mruby support in meson needs to be finished')

mruby = meson.get_compiler('cpp').find_library('mruby')
add_languages('c')

binding_dependencies = [mruby]

binding_headers = files(
    'binding-util.h',
    'disposable-binding.h',
    'flashable-binding.h',
    'binding-types.h',
    'sceneelement-binding.h',
    'viewportelement-binding.h',
    'serializable-binding.h',
    'mrb-ext/file.h',
    'mrb-ext/rwmem.h',
    'mrb-ext/marshal.h'
)

binding_source = files(
    'binding-mruby.cpp',
    'binding-util.cpp',
    'window-binding.cpp',
    'bitmap-binding.cpp',
    'sprite-binding.cpp',
    'font-binding.cpp',
    'viewport-binding.cpp',
    'plane-binding.cpp',
    'audio-binding.cpp',
    'tilemap-binding.cpp',
    'etc-binding.cpp',
    'graphics-binding.cpp',
    'input-binding.cpp',
    'table-binding.cpp',
    'module_rpg.c',
    'mrb-ext/file.cpp',
    'mrb-ext/marshal.cpp',
    'mrb-ext/rwmem.cpp',
    'mrb-ext/kernel.cpp',
    'mrb-ext/time.cpp'
)
