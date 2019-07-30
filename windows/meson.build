win = import('windows')

res = files(
'resource.h',
'icon.ico',
'resource.rc'
)

windows_resources = win.compile_resources('resource.rc', depend_files: res)
