
ver = get_option('mri_version')
binding_dependencies = []
if ver.version_compare('>1.8') == true
    binding_dependencies += dependency('ruby-' + ver)
else
    lib = get_option('ruby_lib')
    binding_dependencies += compiler.find_library(lib)
    if host_system == 'windows'
        if lib.endswith('-static') and (get_option('fix_essentials') == false)
            # if fixing up essentials this'll have been added already
            binding_dependencies += compiler.find_library('wsock32')
        endif
    else
        binding_dependencies += compiler.find_library('dl')
    endif
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

binding_source = [files(
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
)]

if ver.version_compare('<=1.8') == true
    binding_source += files('miniffi-binding.cpp')
endif

bindings = [binding_headers, binding_source]
