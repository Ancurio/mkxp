
embedded_shaders = [
    'common.h',
    'transSimple.frag',
    'trans.frag',
    'hue.frag',
    'sprite.frag',
    'plane.frag',
    'gray.frag',
    'bitmapBlit.frag',
    'flatColor.frag',
    'simple.frag',
    'simpleColor.frag',
    'simpleAlpha.frag',
    'simpleAlphaUni.frag',
    'tilemap.frag',
    'flashMap.frag',
    'minimal.vert',
    'simple.vert',
    'simpleColor.vert',
    'sprite.vert',
    'tilemap.vert',
    'tilemapvx.vert',
    'blur.frag',
    'blurH.vert',
    'blurV.vert',
    'simpleMatrix.vert'
]

embedded_shaders_f = files(embedded_shaders)

count = 0
foreach file : embedded_shaders_f
    global_sources += custom_target(embedded_shaders[count],
        input: file,
        output: '@0@.xxd'.format(embedded_shaders[count]),
        command: [
            xxd, '-i', '@INPUT@'
        ],
        capture: true,
        depend_files: embedded_shaders_f[count]
        )
    count += 1
endforeach
