cmake_minimum_required(VERSION 2.8.11)
Project(mkxp)

## Setup options ##

option(SHARED_FLUID "Dynamically link fluidsynth at build time" OFF)
option(WORKDIR_CURRENT "Keep current directory on startup" OFF)
option(FORCE32 "Force 32bit compile on 64bit OS" OFF)
set(BINDING "MRI" CACHE STRING "The Binding Type (MRI, MRUBY, NULL)")
set(EXTERNAL_LIB_PATH "" CACHE PATH "External precompiled lib prefix")

## Misc setup ##

include(cmake/PrepUtils.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

IF("${CMAKE_SYSTEM}" MATCHES "Linux")
	SET(LINUX ON)
ENDIF()

IF(WORKDIR_CURRENT)
	list(APPEND DEFINES
		WORKDIR_CURRENT
	)
ENDIF()

IF(FORCE32)
	if(APPLE)
		SET(CMAKE_OSX_ARCHITECTURES "i386")
	else()
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	endif()
ENDIF()

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.6)

IF(LINUX)
	if(CMAKE_SIZEOF_VOID_P MATCHES "8" AND NOT(FORCE32) )
		set(CMAKE_EXECUTABLE_SUFFIX ".bin.x86_64")
		set(BIN_RPATH "\$ORIGIN/lib64")
		set(LIB_PATH "lib64")
	else()
		set(CMAKE_EXECUTABLE_SUFFIX ".bin.x86")
		set(BIN_RPATH "\$ORIGIN/lib")
		set(LIB_PATH "lib")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")
	endif()
elseif(APPLE)
	SET(BIN_RPATH "@executable_path/../Frameworks")
	set(LIB_PATH "lib")
endif()

set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH ${BIN_RPATH})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

## Locate core libs ##

find_package(PkgConfig REQUIRED)

if (EXTERNAL_LIB_PATH)
	set(CMAKE_PREFIX_PATH ${EXTERNAL_LIB_PATH})
	
	if(EXISTS "${EXTERNAL_LIB_PATH}/${LIB_PATH}/pkgconfig/")
		SET(ENV{PKG_CONFIG_PATH} "${EXTERNAL_LIB_PATH}/${LIB_PATH}/pkgconfig/")
	endif()
	if(APPLE)
		set(PLATFORM_SHARED_LIBS 
			libSDL2.dylib libSDL2_image-2.0.0.dylib libSDL2_ttf-2.0.0.dylib libSDL_sound-1.0.1.dylib
			libfreetype.6.dylib libsigc-2.0.0.dylib
			CACHE STRING "List of shared libraries that need to be copied into the OS X bundle")
		foreach(lib ${PLATFORM_SHARED_LIBS})
			if(EXISTS ${EXTERNAL_LIB_PATH}/lib/${lib})
				list(APPEND PLATFORM_COPY_LIBS
					${EXTERNAL_LIB_PATH}/lib/${lib}
				)
			endif()
		endforeach()
	endif()
endif()

pkg_check_modules(SIGCXX        REQUIRED sigc++-2.0)
pkg_check_modules(PIXMAN        REQUIRED pixman-1)
pkg_check_modules(PHYSFS        REQUIRED physfs>=2.1)
pkg_check_modules(VORBISFILE    REQUIRED vorbisfile)
pkg_check_modules(SDL2          REQUIRED sdl2)
pkg_check_modules(SDL2_TTF      REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_IMAGE    REQUIRED SDL2_image)
pkg_check_modules(SDL_SOUND     REQUIRED SDL_sound)

find_package(Boost 1.49 COMPONENTS program_options REQUIRED)
find_package(OpenAL REQUIRED)
find_package(ZLIB REQUIRED)

## Setup main source ##

set(MAIN_HEADERS
	src/quadarray.h
	src/audio.h
	src/binding.h
	src/bitmap.h
	src/disposable.h
	src/etc.h
	src/etc-internal.h
	src/eventthread.h
	src/flashable.h
	src/font.h
	src/input.h
	src/plane.h
	src/scene.h
	src/sprite.h
	src/table.h
	src/texpool.h
	src/tilequad.h
	src/transform.h
	src/viewport.h
	src/window.h
	src/serializable.h
	src/shader.h
	src/glstate.h
	src/quad.h
	src/tilemap.h
	src/tilemap-common.h
	src/graphics.h
	src/gl-debug.h
	src/global-ibo.h
	src/exception.h
	src/filesystem.h
	src/serial-util.h
	src/intrulist.h
	src/binding.h
	src/gl-util.h
	src/util.h
	src/config.h
	src/settingsmenu.h
	src/keybindings.h
	src/tileatlas.h
	src/sharedstate.h
	src/al-util.h
	src/boost-hash.h
	src/debugwriter.h
	src/gl-fun.h
	src/gl-meta.h
	src/vertex.h
	src/soundemitter.h
	src/aldatasource.h
	src/alstream.h
	src/audiostream.h
	src/rgssad.h
	src/windowvx.h
	src/tilemapvx.h
	src/tileatlasvx.h
	src/sharedmidistate.h
	src/fluid-fun.h
	src/sdl-util.h
)

set(MAIN_SOURCE
	src/main.cpp
	src/audio.cpp
	src/bitmap.cpp
	src/eventthread.cpp
	src/filesystem.cpp
	src/font.cpp
	src/input.cpp
	src/plane.cpp
	src/scene.cpp
	src/sprite.cpp
	src/table.cpp
	src/tilequad.cpp
	src/viewport.cpp
	src/window.cpp
	src/texpool.cpp
	src/shader.cpp
	src/glstate.cpp
	src/tilemap.cpp
	src/autotiles.cpp
	src/graphics.cpp
	src/gl-debug.cpp
	src/etc.cpp
	src/config.cpp
	src/settingsmenu.cpp
	src/keybindings.cpp
	src/tileatlas.cpp
	src/sharedstate.cpp
	src/gl-fun.cpp
	src/gl-meta.cpp
	src/vertex.cpp
	src/soundemitter.cpp
	src/sdlsoundsource.cpp
	src/alstream.cpp
	src/audiostream.cpp
	src/rgssad.cpp
	src/bundledfont.cpp
	src/vorbissource.cpp
	src/windowvx.cpp
	src/tilemapvx.cpp
	src/tileatlasvx.cpp
	src/autotilesvx.cpp
	src/midisource.cpp
	src/fluid-fun.cpp
)

source_group("MKXP Source" FILES ${MAIN_SOURCE} ${MAIN_HEADERS})

## Setup embedded source ##

set(EMBEDDED_INPUT
	shader/common.h
	shader/transSimple.frag
	shader/trans.frag
	shader/hue.frag
	shader/sprite.frag
	shader/plane.frag
	shader/gray.frag
	shader/bitmapBlit.frag
	shader/flatColor.frag
	shader/simple.frag
	shader/simpleColor.frag
	shader/simpleAlpha.frag
	shader/simpleAlphaUni.frag
	shader/flashMap.frag
	shader/minimal.vert
	shader/simple.vert
	shader/simpleColor.vert
	shader/sprite.vert
	shader/tilemap.vert
	shader/tilemapvx.vert
	shader/blur.frag
	shader/blurH.vert
	shader/blurV.vert
	shader/simpleMatrix.vert
	assets/liberation.ttf
	assets/icon.png
)

if (RGSS2)
	list(APPEND DEFINES
		RGSS2
	)
endif()

if (SHARED_FLUID)
	pkg_check_modules(FLUID REQUIRED fluidsynth)
	list(APPEND DEFINES
		SHARED_FLUID
	)
endif()

## Process Embeddeds ##

find_program(XXD_EXE xxd
	DOC "Location of the xxd executable"
)

macro(ProcessWithXXD outvar inputfile outdir)
	get_filename_component(basefile ${inputfile} NAME)
	set(outputfile ${outdir}/${basefile}.xxd)
	set_source_files_properties(${outputfile} PROPERTIES HEADER_ONLY TRUE)
	add_custom_command(
		OUTPUT ${outputfile}
		COMMAND ${XXD_EXE} -i ${inputfile} ${outputfile}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${inputfile}
		COMMENT "Generating XXD for ${inputfile}"
	)
	list(APPEND ${outvar}
		${outputfile}
	)
endmacro()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/xxdhack)
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/xxdhack
)

foreach(item ${EMBEDDED_INPUT})
	ProcessWithXXD(EMBEDDED_SOURCE ${item} ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

source_group("Embedded Source" FILES ${EMBEDDED_INPUT} ${EMBEDDED_SOURCE})

## Setup binding source ##

if (BINDING STREQUAL "MRI")
	set(MRIVERSION "2.1" CACHE STRING "Version of MRI to link with")
	pkg_check_modules(MRI REQUIRED ruby-${MRIVERSION})
	list(APPEND DEFINES
		BINDING_MRI
	)
	set(BINDING_HEADERS
		binding-mri/binding-util.h
		binding-mri/binding-types.h
		binding-mri/serializable-binding.h
		binding-mri/disposable-binding.h
		binding-mri/sceneelement-binding.h
		binding-mri/viewportelement-binding.h
		binding-mri/flashable-binding.h
	)
	set(BINDING_SOURCE
		binding-mri/binding-mri.cpp
		binding-mri/binding-util.cpp
		binding-mri/table-binding.cpp
		binding-mri/etc-binding.cpp
		binding-mri/bitmap-binding.cpp
		binding-mri/font-binding.cpp
		binding-mri/graphics-binding.cpp
		binding-mri/input-binding.cpp
		binding-mri/sprite-binding.cpp
		binding-mri/viewport-binding.cpp
		binding-mri/plane-binding.cpp
		binding-mri/window-binding.cpp
		binding-mri/tilemap-binding.cpp
		binding-mri/audio-binding.cpp
		binding-mri/module_rpg.cpp
		binding-mri/filesystem-binding.cpp
		binding-mri/windowvx-binding.cpp
		binding-mri/tilemapvx-binding.cpp
	)
elseif(BINDING STREQUAL "MRUBY")
	message(FATAL_ERROR "Mruby support in CMake needs to be finished")
	list(APPEND DEFINES
		BINDING_MRUBY
	)
	set(BINDING_HEADERS
		binding-mruby/binding-util.h
		binding-mruby/disposable-binding.h
		binding-mruby/flashable-binding.h
		binding-mruby/binding-types.h
		binding-mruby/sceneelement-binding.h
		binding-mruby/viewportelement-binding.h
		binding-mruby/serializable-binding.h
		binding-mruby/mrb-ext/file.h
		binding-mruby/mrb-ext/rwmem.h
		binding-mruby/mrb-ext/marshal.h
	)
	set(BINDING_SOURCE
		binding-mruby/binding-mruby.cpp
		binding-mruby/binding-util.cpp
		binding-mruby/window-binding.cpp
		binding-mruby/bitmap-binding.cpp
		binding-mruby/sprite-binding.cpp
		binding-mruby/font-binding.cpp
		binding-mruby/viewport-binding.cpp
		binding-mruby/plane-binding.cpp
		binding-mruby/audio-binding.cpp
		binding-mruby/tilemap-binding.cpp
		binding-mruby/etc-binding.cpp
		binding-mruby/graphics-binding.cpp
		binding-mruby/input-binding.cpp
		binding-mruby/table-binding.cpp
		binding-mruby/module_rpg.c
		binding-mruby/mrb-ext/file.cpp
		binding-mruby/mrb-ext/marshal.cpp
		binding-mruby/mrb-ext/rwmem.cpp
		binding-mruby/mrb-ext/kernel.cpp
		binding-mruby/mrb-ext/time.cpp
	)
elseif(BINDING STREQUAL "NULL")
	set(BINDING_SOURCE
		binding-null/binding-null.cpp
	)
else()
	message(FATAL_ERROR "Must choose a valid binding type.  MRI, MRUBY, or NULL")
endif()

source_group("Binding Source" FILES ${BINDING_SOURCE} ${BINDING_HEADERS})

## Setup main executable ##

if(APPLE)
	find_library(CARBON_LIBRARY Carbon)
	find_library(IOKIT_LIBRARY IOKit)
	mark_as_advanced(CARBON_LIBRARY IOKIT_LIBRARY)
	list(APPEND PLATFORM_LIBRARIES
		${CARBON_LIBRARY}
		${IOKIT_LIBRARY}
		"-liconv"
	)
endif()

link_directories(
	${SIGCXX_LIBRARY_DIRS}
	${PIXMAN_LIBRARY_DIRS}
	${PHYSFS_LIBRARY_DIRS}
	${SDL2_LIBRARY_DIRS} # Blindly assume other SDL bits are in same directory
	${Boost_LIBRARY_DIR}
	${MRI_LIBDIR}
)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
	${MAIN_HEADERS}
	${MAIN_SOURCE}
	${BINDING_HEADERS}
	${BINDING_SOURCE}
	${EMBEDDED_SOURCE}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
	${DEFINES}
)
target_include_directories(${PROJECT_NAME} PRIVATE
	src
	${SIGCXX_INCLUDE_DIRS}
	${PIXMAN_INCLUDE_DIRS}
	${PHYSFS_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIRS} # Blindly assume other SDL bits are in same directory
	${Boost_INCLUDE_DIR}
	${MRI_INCLUDE_DIRS}
	${VORBISFILE_INCLUDE_DIRS}
	${FLUID_INCLUDE_DIRS}
	${OPENAL_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
	${SIGCXX_LIBRARIES}
	${SDL2_LIBRARIES}
	${SDL2_IMAGE_LIBRARIES}
	${SDL2_TTF_LIBRARIES}
	${SDL_SOUND_LIBRARIES}
	${PHYSFS_LIBRARIES}
	${PIXMAN_LIBRARIES}
	${Boost_LIBRARIES}
	${MRI_LIBRARIES}
	${VORBISFILE_LIBRARIES}
	${FLUID_LIBRARIES}
	${OPENAL_LIBRARY}
	${ZLIB_LIBRARY}

	${PLATFORM_LIBRARIES}
)

PostBuildMacBundle(${PROJECT_NAME} "" "${PLATFORM_COPY_LIBS}")
