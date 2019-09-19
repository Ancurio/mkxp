win = import('windows')

conf_data = configuration_data()

if get_option('mk') == true
    conf_data.set('icon', 'icon_mk')
else
    conf_data.set('icon', 'icon')
endif

configure_file(input: 'resource.rc.in', output: 'resource.rc', configuration: conf_data)

res = files(
'resource.h',
'icon.ico',
'resource.rc'
)

windows_resources = win.compile_resources('resource.rc', depend_files: res)
