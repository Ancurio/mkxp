# mkxp-z


![GitHub commits since latest release](https://img.shields.io/github/commits-since/inori-z/mkxp-z/latest?style=for-the-badge)
![built with resentment](https://img.shields.io/badge/built_with-resentment-d55c32?style=for-the-badge)
![Made for dark types](https://img.shields.io/badge/made_for-dark_types-71584a?style=for-the-badge)

![ss](/screenshot.png?raw=true)

This is a fork of mkxp that is intended to run and alleviate the difficulty of porting games based on Pokemon Essentials. It's not necessarily intended to be a byte-for-byte copy of RGSS though, so non-standard extensions and optional enhancements can/will be written for fangame developers (you poor souls) to take advantage of.

## Bindings
Bindings provide the glue code for an interpreted language environment to run game scripts in. mkxp-z focuses on Ruby 1.8 and as such the mruby and null bindings are not included. The original MRI bindings remain for the time being, with the possible intent of working with >=1.9 to better support RGSS3. Please see the original README for more details.

### MRI
Website: https://www.ruby-lang.org/en/

Matz's Ruby Interpreter, also called CRuby, is the most widely deployed version of ruby. MRI 1.8.1 is what was used in RPG Maker XP, and 1.8.7 is what mkxp-z is written around (at least for now). 1.8.1 and 1.8.7 are for the most part identical, though there are a few differences that need to be ironed out before Essentials can be loaded.

This binding should support RGSS1, RGSS2 and RGSS3, though I've only tested it with RGSS1.

## Dependencies / Building

* ObjFW
* Boost.Unordered (headers only)
* Boost.Program_options
* libsigc++ 2.0
* PhysFS (latest hg)
* OpenAL
* SDL2*
* SDL2_image
* SDL2_ttf
* [Ancurio's SDL_sound fork](https://github.com/Ancurio/SDL_sound)
* [My Ruby 1.8 fork](https://github.com/inori-z/ruby/tree/ruby_1_8_7), for Zlib, a Windows build that doesn't segfault, and any 1.8.1 compatibility stuff
* vorbisfile
* pixman
* zlib (only ruby bindings)
* OpenGL header (alternatively GLES2 with `-Dcpp_args=-DGLES2_HEADER`)
* libiconv (on Windows, optional with INI_ENCODING)
* libguess (optional with INI_ENCODING)

(* For the F1 menu to work correctly under Linux/X11, you need latest hg + [this patch](https://bugzilla.libsdl.org/show_bug.cgi?id=2745))

mkxp-z employs the meson build system, so you'll need to install that beforehand.

meson will use pkg-config to locate the respective include/library paths. If you installed any dependencies into non-standard prefixes, make sure to set `-Dpkg_config_path` accordingly when configuring the build. If pkgconfig cannot find a dependency, meson will attempt to use CMake scripts instead (if CMake is installed), followed by system installations/macOS frameworks.

Midi support is enabled by default and requires fluidsynth to be present at runtime (not needed for building); if mkxp can't find it at runtime, midi playback is disabled. It looks for `libfluidsynth.so.1` on Linux, `libfluidsynth.dylib.1` on OSX and `fluidsynth.dll` on Windows, so make sure to have one of these in your link path. If you still need fluidsynth to be hard linked at buildtime, use `-Dshared_fluid=true`. When building fluidsynth yourself, you can disable almost all options (audio drivers etc.) as they are not used. Note that upstream fluidsynth has support for sharing soundfont data between synthesizers (mkxp uses multiple synths), so if your memory usage is very high, you might want to try compiling fluidsynth from git master.

By default, mkxp switches into the directory where its binary is contained and then starts reading the configuration and resolving relative paths. In case this is undesired (eg. when the binary is to be installed to a system global, read-only location), it can be turned off by adding `-Dworkdir_current=true` to meson's build arguments.

To auto detect the encoding of the game title in `Game.ini` and auto convert it to UTF-8, build with `-Dini_encoding=true`. Requires iconv implementation and libguess. If the encoding is wrongly detected, you can set the "titleLanguage" hint in mkxp.conf.

**MRI-Binding**: By default, meson will search for Ruby 1.8 libraries and includes within the system search path. This can be adjusted with `-Dcpp_args=-I[path]` for includes and `-Dcpp_link_args=-L[path]` for libraries. For newer Ruby versions, pkg-config will look for `ruby-X.Y.pc`, where `X` is the major version number and `Y` is the minor version number (e.g. `ruby-2.6.pc`). The version that will be searched for can be set with `-Dmri_version=X.Y` (`-Dmri_version=2.6` as an example).

## Supported image/audio formats

These depend on the SDL auxiliary libraries. For maximum RGSS compliance, build SDL2_image with png/jpg support, and SDL_sound with oggvorbis/wav/mp3 support.

To run mkxp, you should have a graphics card capable of at least **OpenGL (ES) 2.0** with an up-to-date driver installed.

A few notes on compatibility differences compared to RMXP:

* If your graphics are appearing oddly (some pixels appearing transparent when they shouldn't), export in or convert your stuff to RGB instead.
* You will need to re-encode any audio files that OpenAL does not like (16-bit signed WAV works fine, 32-bit float WAV does not, for instance). The game will hitch while attempting to read unsupported formats.
* If you don't know where to even begin with fixing this stuff, ImageMagick is the go-to for images (`convert in.png PNG32:out.png` for converting to a new file, `mogrify -define png:format=png32 in.png` for converting in-place) and FFMPEG is the tool for... well, many things, but it will convert your audio (`ffmpeg -i in.wav out.wav`). You could take advantage of these commands to write scripts that can fix all your stuff for you instead of having to convert them all one-by-one with whatever GUI program you favor. Or you could just use the [crappy one I wrote which will still do the job](https://gist.github.com/inori-z/33c27340fe215814f1890e367e2cab52).

## Configuration

mkxp reads configuration data from the file "mkxp.conf". The format is ini-style. Do *not* use quotes around file paths (spaces won't break). Lines starting with '#' are comments. See 'mkxp.conf.sample' for a list of accepted entries.

All option entries can alternatively be specified as command line options. Any options that are not arrays (eg. RTP paths) specified as command line options will override entries in mkxp.conf. Note that you will have to wrap values containing spaces in quotes (unlike in mkxp.conf).

The syntax is: `--<option>=<value>`

Example: `./mkxp --gameFolder="my game" --vsync=true --fixedFramerate=60`

## Win32API

Win32API exists in mkxp-z as both `Win32API.new` and `MiniFFI.new` (This class is available under macOS, linux and Windows and "Win32API" as a name makes no sense on the former two platforms). It functions nearly the same as Ruby 1.8's Win32API, for better or worse. The third and fourth arguments are now optional (if you just want a function that takes no arguments and returns nothing, for instance), and `new` will yield to blocks. Being simple as it is, it remains mostly as the lazy option/last resort to add C functions from shared libraries if you can't/don't want to build mkxp-z yourself.

### fake-api

mkxp-z provides limited support for some WinAPI functions that would normally break. While using Win32API isn't the most recommended course of action, it does make porting easier if you don't care whether it's used or not. Building with the `use_fakeapi` option enables this.

#### Universal
* `GetCurrentThreadId`: Always returns `571`.
* `GetWindowThreadProcessId`: Always returns `571`.
* `FindWindowEx`: Returns SDL window handle on Windows, else always returns `571`.
* `GetForegroundWindow`: Returns `571` if the window has input focus, else `0`.
* `GetClientRect`: Returns window size.
* `GetCursorPos`: Returns global mouse position.
* `ScreenToClient`: Returns position relative to the game window, taking into account window scale.
* `SetWindowPos`: Sets window position.
* `SetWindowTextA`: Sets the window title.
* `GetWindowRect`: Returns the screen coordinates of the game window's corners.
* `RegisterHotKey`: No-op. pls no disabling SDL's precious fullscreen.
* `SetWindowLong`: Native SetWindowLong on Windows. Everywhere else, only supports switching between fullscreen/windowed modes and always returns `571`.
* `GetKeyboardState`: On Windows, adds states for Shift based on SDL's keystates. Emulated everywhere else.

#### macOS/Linux
* `RtlMoveMemory`: memcpy.
* `LoadLibrary`/`FreeLibrary`: dlopen/dlclose. Obviously will never actually work.
* `GetAsyncKeyState`: Emulated based on SDL mouse/key states.
* `GetSystemPowerStatus`: Emulated based on SDL_GetPowerInfo.
* `ShowWindow`: No-op.
* `GetSystemMetrics`: Only supports getting screen width/height.
* `SetCapture`: No-op. Always returns `571`.
* `ReleaseCapture`: No-op.
* `GetPrivateProfileString`: Emulated with OFINIFile.
* `GetUserDefaultLangId`: Checks for JP, EN, FR, IT, DE, ES, KO, PT and ZH. Returns English (`0x09`) if the locale can't be determined. Doesn't handle sublanguages.
* `GetUserName`: *(macOS)* Gets user login name. *(Linux)* Returns the `$USER` environment variable, or `Ditto` if it doesn't exist.

## Nonstandard RGSS extensions

### Kernel

* `load_data` now has a second optional `Boolean` argument which, if set to true, returns the contents of the file as a string (rather than trying to load it with Marshal first).

### Input

* The `Input.press?` family of functions accepts three additional button constants: `::MOUSELEFT`, `::MOUSEMIDDLE` and `::MOUSERIGHT` for the respective mouse buttons. It will now also accept [SDL scancodes](https://wiki.libsdl.org/SDL_Scancode?highlight=%28%5CbCategoryEnum%5Cb%29%7C%28CategoryKeyboard%29) in the form of symbols corresponding to each scancode (e.g. `SDL_SCANCODE_RETURN` would be requested through `Input.press?/trigger?/repeat? :RETURN`)
* The `Input` module has two additional properties: `text_input` determines whether to accept text editing events. `clipboard` gets and sets the user's clipboard.
* The `Input` module has eight additional functions.
    + `#mouse_x` and `#mouse_y` query the mouse pointer position relative to the game screen. 
    + `#pressex?`, `#triggerex?` and `#repeatex?` provide input states for raw key codes, which are provided in the form of [Microsoft Virtual-Key Codes](https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes). Only buttons which are also tracked by SDL are supported.
    + `#gets` returns a UTF-8 string of any text that was input by the user since the last time `#gets` was called. The `text_input` property must be set to true for it to work.
    + `#joystick` returns a hash containing the `:name` and `:power` entries, or `nil` if there is not a joystick connected. `:name` is a string corresponding to the joystick's name, and `:power` can be any from: `:MAX`, `:WIRED`, `:FULL`, `:MEDIUM`, `:LOW`, `:EMPTY`, and `:UNKNOWN`.
    + `#rumble` triggers a simple rumble effect on supported controllers. It has one required argument (the duration in ms, will stop any active effects when `0`), and three optional arguments (the strength from 0 to 100, the attack envelope duration in ms, and the fade envelope duration in ms)

### Graphics

* RGSS2 Graphics functions and properties are now bound in RGSS1 mode.
* The `Graphics` module has three additional properties: `fullscreen` represents the current fullscreen mode (`true` = fullscreen, `false` = windowed), `show_cursor` hides the system cursor inside the game window when `false`. `scale` represents the current scale factor of the screen, and can be set from `0.5` to `2`.
* The `Graphics` module has two additional functions: `#screenshot(path)` will save a screenshot to `path` in BMP format. `#center` will move the window to the center of the screen.

### Bitmap

* The `Bitmap` class has one additional property: `raw_data` gets and sets the raw pixel data of a Bitmap in the form of a string. The string must be the size of the bitmap's `width*height*4`. If it is not, no error is raised, but the Bitmap will not be updated.
* The `Bitmap` class has one additional function: `to_file(path)` will save the bitmap to `path` in BMP format.

### Audio

* RGSS2 Audio functions and properties are now bound in RGSS1 mode.

### System

The `System` module contains functions that don't really fit anywhere else.

* `data_directory` returns the assigned directory for saves and other data files.
* `set_window_title` sets the window's title.
* `raw_key_states` returns the raw state of MKXP's keystates in the form of a string.
* `mouse_in_window` returns whether the mouse is currently located within the window.
* `platform` returns the operating system MKXP is running on (e.g. `Windows`).
* `user_language` returns the user's current locale (e.g. `en_US`).
* `game_title` returns the game's title as set in its ini.
* `power_state` returns a hash with the system power state information. Its members are `:discharging` (Boolean), `:percent` (int/nil), and `:seconds` (int/nil)
* `show_settings` displays the keybinding menu.
* `nproc` returns the amount of logical cores available to the system.
* `memory` returns the total amount of RAM detected by the system in megabytes.

### Discord

* This module is only included if the `discord_sdk_path` option is set to the location of the Discord GameSDK when building.

```ruby
# Boolean. Whether mkxp-z has connected to a Discord client.
p Discord.connected?

# String. The user's name. Empty if Discord isn't connected.
p Discord.user_name

# String. The user's discriminator. Empty if Discord isn't connected.
p Discord.user_discriminator

# Numeric. The user's ID. Always 0 if Discord isn't connected.
p Discord.user_id

# Bitmap/Nil. The user's avatar. The returned Bitmap is initially empty,
# and updates itself with the avatar once Discord has finished
# downloading it. Nil if Discord isn't connected.

# An optional size argument may also be passed, and defaults to 64.
avatar = Sprite.new
avatar.bitmap = (Discord.connected?) ? Discord.user_avatar : Bitmap.new(64,64)
avatar.visible = true

# Create a new Activity. If passed a block, the Activity will automatically
# attempt to send itself after execution of the block.
activity = Discord::Activity.new

# Available properties:
# - state
# - details
# - large_image
# - large_text
# - small_image
# - small_text
# - party_id
# - party_currentsize
# - party_maxsize
# - type (0: Playing, 1: Streaming, 2: Listening, 3: Watching)
# - start_time
# - end_time
# - instance

# Send the Activity. Does nothing if Discord isn't connected.
activity.send
```

## Platform-specific scripts

By including `|p|` or `|!p|` at the very beginning of a script's title, you can specify what platform the script should or shouldn't run on. `p` should match the first letter of your platform, and is case-insensitive. As an example, a script named `|!W| Socket` would not run on Windows, while a script named `|W| Socket` would *only* run on Windows.

## Midi music

mkxp doesn't come with a soundfont by default, so you will have to supply it yourself (set its path in the config). Playback has been tested and should work reasonably well with all RTP assets.

You can use this public domain soundfont: [GMGSx.sf2](https://www.dropbox.com/s/qxdvoxxcexsvn43/GMGSx.sf2?dl=0)

## Fonts

In the RMXP version of RGSS, fonts are loaded directly from system specific search paths (meaning they must be installed to be available to games). Because this whole thing is a giant platform-dependent headache, Ancurio decided to implement the behavior Enterbrain thankfully added in VX Ace: loading fonts will automatically search a folder called "Fonts", which obeys the default searchpath behavior (ie. it can be located directly in the game folder, or an RTP).

If a requested font is not found, no error is generated. Instead, a built-in font is used. By default, this font is Liberation Sans. WenQuanYi MicroHei is used as the built-in font if the `cjk_fallback_font` option is used.

## What doesn't work (yet)

* The current implementation of `load_data` is case-sensitive. If you try to load `Data/MapXXX`, you will not find `Data/mapXXX`.
* `load_data` is slow. In fact, it's too slow to handle `pbResolveBitmap` firing a million times a second, so Graphics files can only be loaded from outside of the game's archive. You could remove that code if you want, but you'll lag if not using loose files. Very hard.
* `FileSystem::openRead` is not compatible with absolute paths in Windows (this affects `Bitmap.new` or `Audio.bgm_play`, for instance).
* Movie playback
* wma audio files
* Creating Bitmaps with sizes greater than the OpenGL texture size limit (around 16384 on modern cards)^

^ There is an exception to this, called *mega surface*. When a Bitmap bigger than the texture limit is created from a file, it is not stored in VRAM, but regular RAM. Its sole purpose is to be used as a tileset bitmap. Any other operation to it (besides blitting to a regular Bitmap) will result in an error.

-------------------------

## Building on Windows (MSYS+MinGW)

1. Install [MSYS](https://www.msys2.org) and launch it. Run `pacman -Syu`, and close the console when it asks you to, then launch MSYS using the `MSYS MinGW 32-bit` shortcut that’s been added to your Start menu. Run `pacman -Su` this time.

2. Add `C:\msys64\mingw32\bin` to your PATH. Unless you want to try and build mkxp-z statically, this is important if you want to actually run the program you built. Don't forget to log out and back in.

3. Install most of the dependencies you need with pacman:

```sh
pacman -S base-devel git mingw-w64-i686-{gcc,meson,cmake,openal,boost,SDL2,pixman,physfs,libsigc++,libvorbis,fluidsynth,libiconv,libguess} \
mingw-w64-i686-SDL2_{image,ttf}
```

4. Install the rest from source:

```sh
mkdir src; cd src
git clone https://github.com/Ancurio/SDL_Sound
git clone https://github.com/ObjFW/ObjFW
git clone https://github.com/inori-z/ruby --single-branch --branch ruby_1_8_7

cd SDL_Sound && ./bootstrap
./configure --enable-{modplug,speex,flac}=no
make install -j`nproc`

cd ../ObjFW
./autogen.sh
./configure
make install -j`nproc`

cd ../ruby

# when you install ruby, some extensions might not want to build.
# You probably don’t particularly need any, so you can just delete
# any problematic ones if you like:

rm -rf ext/{tk,win32ole,openssl}

# and try building again afterwards. Or you can try to fix whatever
# the problem is (missing libraries, usually). Hey, do whatever you need to
# do, Capp’n.

autoconf
./configure --enable-shared --disable-install-doc
make -j`nproc` && make install
cd ..

# `make install` runs a ruby script which apparently does not comply
# with MSYS’s unix paths, so everything Ruby installs will go to the 
# root of your drive. You have to move the resulting `mingw32` 
# folder to the msys64 root yourself.
```

5. Build mkxp-z:

```sh
git clone https://github.com/inori-z/mkxp-z
cd mkxp-z

# Ruby 1.8 doesn’t support pkg-config (Might add it in, since this
# is a bit annoying) so you have to set include and link paths yourself.
# for mingw32, includes will be at /mingw32/lib/ruby/1.8/i386-mingw32

meson build -Druby_lib=msvcrt-ruby18 -Duse_fakeapi=true \
-Dcpp_args=-I/mingw32/lib/ruby/1.8/i386-mingw32

cd build
ninja
```

You'll find your stuff under your MSYS home directory. You can also type `explorer .` in the shell to take a shortcut to the results.

## Building on macOS (homebrew)

1. Install [Homebrew](https://brew.sh).

2. Get most of your dependencies from Homebrew:

```sh
brew install meson cmake automake autoconf sdl2 sdl2_{image,ttf} objfw \
boost pixman physfs libsigc++ libvorbis fluidsynth pkgconfig libtool
```

3. Build the rest from source:

```sh
mkdir ~/src; cd ~/src
git clone https://github.com/Ancurio/SDL_Sound
git clone https://github.com/inori-z/ruby --single-branch --branch ruby_1_8_7

cd SDL_Sound && ./bootstrap
./configure
make install -j`nproc`

cd ../ruby

# when you install ruby, some extensions might not want to build.
# You probably don’t particularly need any, so you can just delete
# any problematic ones if you like:

rm -rf ext/tk

# and try building again afterwards. Or you can try to fix whatever
# the problem is (missing libraries, usually). Hey, do whatever you
# need to do, Capp’n.

autoconf

# We're putting our build of ruby in its own prefix. macOS already
# includes Ruby (at least until 10.16 or so), and you really do
# not want to have any conflicts with it

./configure --prefix=/usr/local/opt/mkxp-ruby --enable-shared --disable-install-doc
make -j`nproc` && make install
cd ..
```

4. Build mkxp-z:

```sh
git clone https://github.com/inori-z/mkxp-z
cd mkxp-z

# Ruby 1.8 doesn’t support pkg-config (Might add it in, since this
# is a bit annoying) so you have to set include and link paths yourself.
# the header folder that Ruby 1.8 creates for macOS includes a version 
# number (i.e. `x86_64-darwin19.0.0`), and is going to be located under
# `/usr/local/opt/mkxp-ruby/lib/ruby/1.8`.

# You will also need to link to `/usr/local/opt/mkxp-ruby/lib`, and tell
# meson where your pkgconfig path is (`/usr/local/lib/pkgconfig`).

meson build -Dpkg_config_path=/usr/local/lib/pkgconfig \
-Dcpp_args=-I/usr/local/opt/mkxp-ruby/lib/ruby/1.8/x86_64-darwin19.0.0 \
-Dobjcpp_link_args=-L/usr/local/opt/mkxp-ruby/lib

cd build
ninja
```

Your results will be in `~/src/mkxp-z/build` . You can type `open ~/src/mkxp-z/build` to get there quickly.


## Building on Linux (Ubuntu Eoan)

> I'm assuming that if you're using anything other than Ubuntu, you're probably familiar enough with this sort of thing to not need instructions. In fact, you've probably built this thing already, haven't you?

1. Open your terminal and make sure your apt cache and packages are up to date with `sudo apt update; sudo apt upgrade`

2. Get most of the dependencies from apt:

```sh
sudo apt install build-essential git bison cmake meson autoconf libtool \
pkg-config xxd libsdl2* libvorbisfile3 libvorbis-dev libpixman-1* \
libboost-program-options1.65* libopenal1*  libopenal-dev zlib1g* \
fluidsynth libfluidsynth-dev libsigc++-2.0* libogg-dev
```

3. Build the rest from source:

```sh
mkdir ~/src; cd ~/src

# pkgconfig won't detect PhysFS unless you build it yourself,
# so...

git clone https://github.com/criptych/physfs
cd physfs
mkdir build; cd build
cmake ..
make -j`nproc`
sudo make install
cd ../..

# Now we can do the other stuff:

git clone https://github.com/Ancurio/SDL_Sound
git clone https://github.com/ObjFW/ObjFW
git clone https://github.com/inori-z/ruby --single-branch --branch ruby_1_8_7

cd SDL_Sound && ./bootstrap
./configure
make -j`nproc`
sudo make install

cd ../ObjFW
./autogen.sh
./configure
make install -j`nproc`

cd ../ruby && autoconf
./configure --enable-shared --disable-install-doc
make -j`nproc`
sudo make install
cd ..
```

4. Build mkxp-z:

```sh
git clone https://github.com/inori-z/mkxp-z
cd mkxp-z

# Ruby 1.8 doesn’t support pkg-config (Might add it in, since this
# is a bit annoying) so you have to set include and link paths yourself.
# in this case, includes will be found at `/usr/local/lib/ruby/1.8/x86_64-linux`.

meson build -Dcpp_args=-I/usr/local/lib/ruby/1.8/x86_64-linux

cd build
ninja
```

See your results with `nautilus ~/src/mkxp-z/build`.
