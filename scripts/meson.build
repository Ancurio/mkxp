
embedded_scripts = [
    'EssentialsCompatibility.rb'
]

embedded_scripts_f = files(embedded_scripts)

count = 0
foreach file : embedded_scripts_f
    global_sources += custom_target(embedded_scripts[count],
        input: file,
        output: '@0@.xxd'.format(embedded_scripts[count]),
        command: [
            xxd, '-i', '@INPUT@'
        ],
        capture: true,
        depend_files: embedded_scripts_f[count]
        )
    count += 1
endforeach
