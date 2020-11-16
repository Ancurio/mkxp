install_data('icon.icns', install_dir: 'Contents/Resources')
install_data('Info.plist', install_dir: 'Contents')

swarg = ''
if steamworks == true
  swarg = steam_libpath
endif

meson.add_install_script('macpack.sh', swarg, meson.project_name())

if get_option('codesign') == true
  meson.add_install_script('codesign.sh', get_option('codesign_identity'))
endif