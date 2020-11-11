#-- Need this for link line stuff?
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0009 NEW)
endif(COMMAND cmake_policy)

# gp_item_default_embedded_path item default_embedded_path_var
#
# Return the path that others should refer to the item by when the item
# is embedded inside a bundle.
#
# Override on a per-project basis by providing a project-specific
# gp_item_default_embedded_path_override function.
#
function(gp_item_default_embedded_path_override item default_embedded_path_var)
	#
	# The assumption here is that all executables in the bundle will be
	# in same-level-directories inside the bundle. The parent directory
	# of an executable inside the bundle should be MacOS or a sibling of
	# MacOS and all embedded paths returned from here will begin with
	# "@loader_path/../" and will work from all executables in all
	# such same-level-directories inside the bundle.
	#

	# By default, embed things right next to the main bundle executable:
	#
	set (install_name_prefix "@executable_path")
	# -------------------------------------------------------------------
	# If your application uses plugins then you should consider using the following
	#  instead but will limit your deployment to OS X 10.4. There is also a patch
	#  needed for CMake that as of Sept 30, 2008 has NOT been applied to CMake.
	#  set (install_name_prefix "@loader_path")
	
	set(path "${install_name_prefix}/../../Contents/MacOS")

	set(overridden 0)

	# Embed .dylibs in the Libraries Directory
	#
	if(item MATCHES "\\.dylib$")
		set(path "${install_name_prefix}/../Libraries")
		set(overridden 1)
	endif(item MATCHES "\\.dylib$")

	# Embed .so files in the Plugins directory
	#
	if(item MATCHES "\\.so$")
		set(path "${install_name_prefix}/../Plugins")
		set(overridden 1)
	endif(item MATCHES "\\.so$")
	
	# Embed frameworks in the embedded "Frameworks" directory (sibling of MacOS):
	#
	if(NOT overridden)
		if(item MATCHES "[^/]+\\.framework/")
			set(path "${install_name_prefix}/../Frameworks")
			set(overridden 1)
		endif(item MATCHES "[^/]+\\.framework/")
	endif(NOT overridden)

	set(${default_embedded_path_var} "${path}" PARENT_SCOPE)
endfunction(gp_item_default_embedded_path_override)

# -- Copy the App bundle to the installation location first
EXECUTE_PROCESS(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/patches/macos/CreateBundle.sh")

# -- Run the BundleUtilities cmake code
include(BundleUtilities)
set(BU_CHMOD_BUNDLE_ITEMS ON)
fixup_bundle("${CMAKE_CURRENT_SOURCE_DIR}/mkxp.app" "" "${CMAKE_CURRENT_SOURCE_DIR}")
execute_process(COMMAND chmod 0700 "${CMAKE_CURRENT_SOURCE_DIR}/mkxp.app")
execute_process(COMMAND sh -c "cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/patches/macos/mkxp.conf ${CMAKE_CURRENT_SOURCE_DIR}/mkxp.app/Contents/Resources/mkxp.conf")