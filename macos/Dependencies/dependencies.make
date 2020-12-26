SDKROOT := $(shell xcrun -sdk $(SDK) --show-sdk-path)
TARGETFLAGS := $(TARGETFLAGS) -m$(SDK)-version-min=$(MINIMUM_REQUIRED)
DEPLOYMENT_TARGET_ENV := $(shell ruby -e 'puts "$(SDK)".upcase')_DEPLOYMENT_TARGET=$(MINIMUM_REQUIRED)
BUILD_PREFIX := ${PWD}/build-$(SDK)-$(ARCH)
LIBDIR := $(BUILD_PREFIX)/lib
INCLUDEDIR := $(BUILD_PREFIX)/include
DOWNLOADS := ${PWD}/downloads/$(HOST)
NPROC := $(shell sysctl -n hw.ncpu)
CFLAGS := -I$(INCLUDEDIR) $(TARGETFLAGS) $(DEFINES)
LDFLAGS := -L$(LIBDIR)
CC      := xcrun -sdk $(SDK) clang -arch $(ARCH) -isysroot $(SDKROOT)
PKG_CONFIG_LIBDIR := $(BUILD_PREFIX)/lib/pkgconfig
GIT := git
CLONE := $(GIT) clone -q
GITHUB := https://github.com
GITLAB := https://gitlab.com

CONFIGURE_ENV := \
	$(DEPLOYMENT_TARGET_ENV) \
	PKG_CONFIG_LIBDIR=$(PKG_CONFIG_LIBDIR) \
	CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"

CONFIGURE_ARGS := \
	--prefix="$(BUILD_PREFIX)" \
	--host=$(HOST)

CMAKE_ARGS := \
	-DCMAKE_INSTALL_PREFIX="$(BUILD_PREFIX)" \
	-DCMAKE_OSX_SYSROOT=$(SDKROOT) \
	-DCMAKE_OSX_ARCHITECTURES=$(ARCH) \
	-DCMAKE_OSX_DEPLOYMENT_TARGET=$(MINIMUM_REQUIRED) \
	-DCMAKE_C_FLAGS="$(CFLAGS)" 

RUBY_CONFIGURE_ARGS := \
	--enable-install-static-library \
	--enable-shared \
	--disable-install-doc \
	--with-out-ext=openssl,fiddle,gdbm,win32ole,win32 \
	--disable-rubygems \
	--with-static-linked-ext \
	${EXTRA_RUBY_CONFIG_ARGS}

CONFIGURE := $(CONFIGURE_ENV) ./configure $(CONFIGURE_ARGS)
AUTOGEN   := $(CONFIGURE_ENV) ./autogen.sh $(CONFIGURE_ARGS)
CMAKE     := $(CONFIGURE_ENV) cmake .. $(CMAKE_ARGS)

default:

# Vorbis
libvorbis: init_dirs libogg $(LIBDIR)/libvorbis.a

$(LIBDIR)/libvorbis.a: $(LIBDIR)/libogg.a $(DOWNLOADS)/vorbis/Makefile
	cd $(DOWNLOADS)/vorbis; \
	make -j$(NPROC); make install

$(DOWNLOADS)/vorbis/Makefile: $(DOWNLOADS)/vorbis/configure
	cd $(DOWNLOADS)/vorbis; \
	$(CONFIGURE) --with-ogg=$(BUILD_PREFIX) --enable-shared=false --enable-static=true

$(DOWNLOADS)/vorbis/configure: $(DOWNLOADS)/vorbis/autogen.sh
	cd $(DOWNLOADS)/vorbis; \
	./autogen.sh

$(DOWNLOADS)/vorbis/autogen.sh:
	$(CLONE) $(GITLAB)/mkxp-z/vorbis $(DOWNLOADS)/vorbis


# Ogg, dependency of Vorbis
libogg: init_dirs $(LIBDIR)/libogg.a

$(LIBDIR)/libogg.a: $(DOWNLOADS)/ogg/Makefile
	cd $(DOWNLOADS)/ogg; \
	make -j$(NPROC); make install

$(DOWNLOADS)/ogg/Makefile: $(DOWNLOADS)/ogg/configure
	cd $(DOWNLOADS)/ogg; \
	$(CONFIGURE) --enable-static=true --enable-shared=false

$(DOWNLOADS)/ogg/configure: $(DOWNLOADS)/ogg/autogen.sh
	cd $(DOWNLOADS)/ogg; ./autogen.sh

$(DOWNLOADS)/ogg/autogen.sh:
	$(CLONE) $(GITLAB)/mkxp-z/ogg $(DOWNLOADS)/ogg

# sigc++-2
sigcxx: init_dirs $(LIBDIR)/libsigc-2.0.a

$(LIBDIR)/libsigc-2.0.a: $(DOWNLOADS)/sigcxx/Makefile
	cd $(DOWNLOADS)/sigcxx; \
	make; make install

$(DOWNLOADS)/sigcxx/Makefile: $(DOWNLOADS)/sigcxx/autogen.sh
	cd $(DOWNLOADS)/sigcxx; \
	$(AUTOGEN) --enable-static=yes --enable-shared=no

$(DOWNLOADS)/sigcxx/autogen.sh:
	$(CLONE) $(GITLAB)/mkxp-z/libsigcplusplus -b libsigc++-2-10 $(DOWNLOADS)/sigcxx

# Pixman
pixman: init_dirs libpng $(LIBDIR)/libpixman-1.a

$(LIBDIR)/libpixman-1.a: $(DOWNLOADS)/pixman/Makefile
	cd $(DOWNLOADS)/pixman
	make -C $(DOWNLOADS)/pixman -j$(NPROC)
	make -C $(DOWNLOADS)/pixman install

$(DOWNLOADS)/pixman/Makefile: $(DOWNLOADS)/pixman/autogen.sh
	cd $(DOWNLOADS)/pixman; \
	$(AUTOGEN) --enable-static=yes --enable-shared=no

$(DOWNLOADS)/pixman/autogen.sh:
	$(CLONE) $(GITLAB)/mkxp-z/pixman $(DOWNLOADS)/pixman


# PhysFS

physfs: init_dirs $(LIBDIR)/libphysfs.a

$(LIBDIR)/libphysfs.a: $(DOWNLOADS)/physfs/cmakebuild/Makefile
	cd $(DOWNLOADS)/physfs/cmakebuild; \
	make -j$(NPROC); make install

$(DOWNLOADS)/physfs/cmakebuild/Makefile: $(DOWNLOADS)/physfs/CMakeLists.txt
	cd $(DOWNLOADS)/physfs; \
	mkdir cmakebuild; cd cmakebuild; \
	$(CMAKE) -DPHYSFS_BUILD_STATIC=true -DPHYSFS_BUILD_SHARED=false

$(DOWNLOADS)/physfs/CMakeLists.txt:
	$(CLONE) $(GITLAB)/mkxp-z/physfs $(DOWNLOADS)/physfs

# libpng
libpng: init_dirs $(LIBDIR)/libpng.a

$(LIBDIR)/libpng.a: $(DOWNLOADS)/libpng/Makefile
	cd $(DOWNLOADS)/libpng; \
	make -j$(NPROC); make install

$(DOWNLOADS)/libpng/Makefile: $(DOWNLOADS)/libpng/configure
	cd $(DOWNLOADS)/libpng; \
	$(CONFIGURE) \
	--enable-shared=no --enable-static=yes

$(DOWNLOADS)/libpng/configure:
	$(CLONE) $(GITLAB)/mkxp-z/libpng $(DOWNLOADS)/libpng

# libjpeg
libjpeg: init_dirs $(LIBDIR)/libjpeg.a

$(LIBDIR)/libjpeg.a: $(DOWNLOADS)/libjpeg/cmakebuild/Makefile
	cd $(DOWNLOADS)/libjpeg/cmakebuild; \
	make -j$(NPROC); make install

$(DOWNLOADS)/libjpeg/cmakebuild/Makefile: $(DOWNLOADS)/libjpeg/CMakeLists.txt
	cd $(DOWNLOADS)/libjpeg; mkdir -p cmakebuild; cd cmakebuild; \
	$(CMAKE) -DENABLE_SHARED=no -DENABLE_STATIC=yes

$(DOWNLOADS)/libjpeg/CMakeLists.txt:
	$(CLONE) $(GITLAB)/mkxp-z/libjpeg-turbo $(DOWNLOADS)/libjpeg

# SDL2
sdl2: init_dirs $(LIBDIR)/libSDL2.a

$(LIBDIR)/libSDL2.a: $(DOWNLOADS)/sdl2/Makefile
	cd $(DOWNLOADS)/sdl2; \
	make -j$(NPROC); make install;

$(DOWNLOADS)/sdl2/Makefile: $(DOWNLOADS)/sdl2/configure
	cd $(DOWNLOADS)/sdl2; \
	$(CONFIGURE) --enable-static=true --enable-shared=false $(SDL_FLAGS)

$(DOWNLOADS)/sdl2/configure: $(DOWNLOADS)/sdl2/autogen.sh
	cd $(DOWNLOADS)/sdl2; ./autogen.sh

$(DOWNLOADS)/sdl2/autogen.sh:
	$(CLONE) $(GITLAB)/mkxp-z/SDL $(DOWNLOADS)/sdl2 -b mkxp-z; cd $(DOWNLOADS)/sdl2

# SDL2 (Image)
sdl2image: init_dirs sdl2 libpng libjpeg $(LIBDIR)/libSDL2_image.a

$(LIBDIR)/libSDL2_image.a: $(DOWNLOADS)/sdl2_image/Makefile
	cd $(DOWNLOADS)/sdl2_image; \
	make -j$(NPROC); make install

$(DOWNLOADS)/sdl2_image/Makefile: $(DOWNLOADS)/sdl2_image/configure
	cd $(DOWNLOADS)/sdl2_image; \
	LIBPNG_LIBS=$(LIBDIR)/libpng.a \
	$(CONFIGURE) --enable-static=true --enable-shared=false \
	--disable-imageio \
	--enable-png=yes --enable-png-shared=no \
	--enable-jpg=yes --enable-jpg-shared=no \
	--enable-webp=no $(SDL2_IMAGE_FLAGS)

$(DOWNLOADS)/sdl2_image/configure: $(DOWNLOADS)/sdl2_image/autogen.sh
	cd $(DOWNLOADS)/sdl2_image; ./autogen.sh

$(DOWNLOADS)/sdl2_image/autogen.sh:
	$(CLONE) $(GITLAB)/mkxp-z/SDL_image $(DOWNLOADS)/sdl2_image -b mkxp-z

# SDL2 (ttf)
sdl2ttf: init_dirs sdl2 freetype $(LIBDIR)/libSDL2_ttf.a

$(LIBDIR)/libSDL2_ttf.a: $(DOWNLOADS)/sdl2_ttf/Makefile
	cd $(DOWNLOADS)/sdl2_ttf; \
	make -j$(NPROC); make install

$(DOWNLOADS)/sdl2_ttf/Makefile: $(DOWNLOADS)/sdl2_ttf/configure
	cd $(DOWNLOADS)/sdl2_ttf; \
	$(CONFIGURE) --enable-static=true --enable-shared=false $(SDL2_TTF_FLAGS)

$(DOWNLOADS)/sdl2_ttf/configure: $(DOWNLOADS)/sdl2_ttf/autogen.sh
	cd $(DOWNLOADS)/sdl2_ttf; ./autogen.sh

$(DOWNLOADS)/sdl2_ttf/autogen.sh:
	$(CLONE) $(GITLAB)/mkxp-z/SDL_ttf $(DOWNLOADS)/sdl2_ttf -b mkxp-z

# Freetype (dependency of SDL2_ttf)
freetype: init_dirs $(LIBDIR)/libfreetype.a

$(LIBDIR)/libfreetype.a: $(DOWNLOADS)/freetype/Makefile
	cd $(DOWNLOADS)/freetype; \
	make -j$(NPROC); make install

$(DOWNLOADS)/freetype/Makefile: $(DOWNLOADS)/freetype/configure
	cd $(DOWNLOADS)/freetype; \
	$(CONFIGURE) --enable-static=true --enable-shared=false

$(DOWNLOADS)/freetype/configure: $(DOWNLOADS)/freetype/autogen.sh
	cd $(DOWNLOADS)/freetype; ./autogen.sh

$(DOWNLOADS)/freetype/autogen.sh:
	$(CLONE) $(GITLAB)/mkxp-z/freetype2 $(DOWNLOADS)/freetype

# OpenAL
openal: init_dirs libogg $(LIBDIR)/libopenal.a

$(LIBDIR)/libopenal.a: $(DOWNLOADS)/openal/cmakebuild/Makefile
	cd $(DOWNLOADS)/openal/cmakebuild; \
	make -j$(NPROC); make install

$(DOWNLOADS)/openal/cmakebuild/Makefile: $(DOWNLOADS)/openal/CMakeLists.txt
	cd $(DOWNLOADS)/openal; mkdir cmakebuild; cd cmakebuild; \
	$(CMAKE) -DLIBTYPE=STATIC -DALSOFT_EXAMPLES=no -DALSOFT_UTILS=no $(OPENAL_FLAGS)

$(DOWNLOADS)/openal/CMakeLists.txt:
	$(CLONE) $(GITLAB)/mkxp-z/openal-soft $(DOWNLOADS)/openal

# Standard ruby
ruby: init_dirs $(LIBDIR)/libruby*.a

$(LIBDIR)/libruby*.a: $(DOWNLOADS)/ruby/Makefile
	cd $(DOWNLOADS)/ruby; \
	make -j$(NPROC); make install

$(DOWNLOADS)/ruby/Makefile: $(DOWNLOADS)/ruby/configure
	cd $(DOWNLOADS)/ruby; \
	$(CONFIGURE) $(RUBY_CONFIGURE_ARGS) $(RUBY_FLAGS)

$(DOWNLOADS)/ruby/configure: $(DOWNLOADS)/ruby/*.c
	cd $(DOWNLOADS)/ruby; autoconf

$(DOWNLOADS)/ruby/*.c:
	$(CLONE) $(GITLAB)/mkxp-z/ruby $(DOWNLOADS)/ruby; \
	cd $(DOWNLOADS)/ruby; git checkout v3_0_0

# Old geezer ruby
legacy-ruby: init_dirs $(LIBDIR)/libruby.a

$(LIBDIR)/libruby.a: $(DOWNLOADS)/ruby18/Makefile
	cd $(DOWNLOADS)/ruby18; \
	make -j$(NPROC); make install

$(DOWNLOADS)/ruby18/Makefile: $(DOWNLOADS)/ruby18/configure
	cd $(DOWNLOADS)/ruby18; \
	$(CONFIGURE) \
	$(RUBY_FLAGS)

$(DOWNLOADS)/ruby18/configure: $(DOWNLOADS)/ruby18/*.c
	cd $(DOWNLOADS)/ruby18; autoconf

$(DOWNLOADS)/ruby18/*.c:
	$(CLONE) $(GITLAB)/mkxp-z/ruby --single-branch --branch ruby_1_8_7 $(DOWNLOADS)/ruby18; \
	rm -rf $(DOWNLOADS)/ruby18/ext/tk

# Build your own ruby!
RUBY_PATH := ${RUBY_PATH}
custom-ruby: custom-ruby-makefile
	cd $(RUBY_PATH); \
	make -j$(NPROC); make install

custom-ruby-makefile: custom-ruby-configure
	cd $(RUBY_PATH); $(CONFIGURE) $(RUBY_FLAGS)

custom-ruby-configure: $(RUBY_PATH)/*.c
	cd $(RUBY_PATH); autoconf


# ====
init_dirs:
	@mkdir -p $(LIBDIR) $(INCLUDEDIR)

clean: clean-compiled

powerwash: clean-compiled clean-downloads

clean-downloads:
	-rm -rf downloads/$(HOST)

clean-compiled:
	-rm -rf build-$(SDK)-$(ARCH)

deps-core: libvorbis sigcxx pixman libpng libjpeg physfs sdl2 sdl2image sdl2ttf openal
everything: deps-core ruby
legacy: deps-core legacy-ruby