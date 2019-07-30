# This will create an Ubuntu 18.04 image with all the dependencies required to build MKXP-EX for linux.

FROM ubuntu:bionic
#--------------------
# Get stuff from apt

RUN apt-get update
RUN apt-get install curl \
	build-essential git bison cmake meson autoconf libtool pkg-config xxd \
	libsdl2* libvorbisfile3 libvorbis-dev libpixman-1* libsigc++-2.0* libogg-dev \
	libboost-program-options1.65* libopenal1 libopenal-dev zlib1g* fluidsynth \
	libfluidsynth-dev -y

#--------------------
RUN mkdir /source

# Install stock Ruby (2.5 for bionic)
RUN apt-get install ruby ruby-dev -y

# Install stock mruby
RUN apt-get install mruby libmruby-dev

# Install stock Ruby 1.8
RUN curl https://ftp.ruby-lang.org/pub/ruby/1.8/ruby-1.8.7.tar.bz2 | tar xj -C /source
RUN (cd /source/ruby-1.8.7 && autoconf && ./configure --disable-install-doc --enable-shared && make && make install)
RUN mkdir -p /usr/local/include/ruby; cp -r /source/ruby-1.8.7/*.h /usr/local/include/ruby

# Install PhysFS
RUN git clone https://github.com/criptych/physfs /source/physfs
RUN mkdir /source/physfs/build; (cd /source/physfs/build && cmake .. && make && make install)

# Install SDL_Sound
RUN git clone https://github.com/Ancurio/SDL_Sound /source/SDL_Sound
RUN (cd /source/SDL_Sound && ./bootstrap && ./configure && make install)

RUN rm -rf /source
