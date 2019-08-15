embedded_assets = [
    'icon.png',
    'icon.svg',
    'liberation.ttf'
]

embedded_assets_f = files(embedded_assets)

count = 0
processed_assets = []
foreach file : embedded_assets_f
    processed_assets += custom_target(embedded_assets[count],
    input: file,
    output: '@0@.xxd'.format(embedded_assets[count]),
    command: [
        xxd, '-i', '@INPUT@'
    ],
    capture: true,
    depend_files: embedded_assets_f[count]
    )
    count += 1
endforeach
