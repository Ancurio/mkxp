install_data('AppRun')
install_data('mkxp-z.png')
install_data('mkxp-z.desktop')

swarg = ''
if steamworks == true
  swarg = steam_libpath
endif
meson.add_install_script('make_appimg.sh', meson.project_name(), get_option('appimagekit_path'), swarg)
