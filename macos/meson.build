if get_option('mk') == false
    install_data('icon.icns', install_dir: 'Contents/Resources')
else
    install_data('icon_mk.icns', install_dir: 'Contents/Resources', rename: 'icon.icns')
endif

install_data('Info.plist', install_dir: 'Contents')

meson.add_install_script('macpack.sh')
