# mkxp

mkxp is a project that seeks to provide a fully open source implementation of the Ruby Game Scripting System (RGSS) interface used in the popular game creation software "RPG Maker XP" (trademark by Enterbrain, Inc.), with focus on Linux. The goal is to be able to run games created with the above software natively without changing a single file. Experimental support for RPG Maker VX / VX Ace games is present.

It is licensed under the GNU General Public License v2+.

## RGSS2 / RGSS3
Support for these RGSS versions is experimental. A lot is implemented (to the point that you can play the VX Ace sample game, with caveats), but not everything is in place yet.

Missing RGSS3 functionality:

* Text outline
* Movie playback
* Audio fade-in

Some other things might be implemented, but simply not bound yet.

RGSS2 might work a little bit less than 3 because I don't have a VX license and as such am unable to experiment thoroughly on it.

## Bindings
Bindings provide the glue code for an interpreted language environment to run game scripts in. Currently there are three bindings:

### MRI
Website: https://www.ruby-lang.org/en/

Matz's Ruby Interpreter, also called CRuby, is the most widely deployed version of ruby. If you're interested in running games created with RPG Maker XP, this is the one you should go for. MRI 1.8 is what was used in RPG Maker XP, however, this binding is written against 2.0 (the latest version). For games utilizing only the default scripts provided by Enterbrain, this binding works quite well so far. Note that there are language and syntax differences between 1.8 and 2.0, so some user created scripts may not work correctly.

For a list of differences, see:
http://stackoverflow.com/questions/21574/what-is-the-difference-between-ruby-1-8-and-ruby-1-9

### mruby (Lightweight Ruby)
Website: https://github.com/mruby/mruby

mruby is a new endeavor by Matz and others to create a more lightweight, spec-adhering, embeddable Ruby implementation. You can think of it as a Ruby version of Lua.

Due to heavy differences between mruby and MRI as well as lacking modules, running RPG Maker games with this binding will most likely not work correctly. It is provided as experimental code. You can eg. write your own ruby scripts and run them.

Some extensions to the standard classes/modules are provided, taking the RPG Maker XP helpfile as a quasi "reference". These include Marshal, File, FileTest and Time.

**Important:** If you decide to use [mattn's oniguruma regexp gem](https://github.com/mattn/mruby-onig-regexp), don't forget to add `-lonig` to the linker flags to avoid ugly symbol overlaps with libc.

### null
This binding only exists for testing purposes and does nothing (the engine quits immediately). It can be used to eg. run a minimal RGSS game loop directly in C++.

## Dependencies / Building

* Boost.Unordered (headers only)
* Boost.Program_options
* libsigc++ 2.0
* PhysFS (latest hg)
* OpenAL
* SDL2
* SDL2_image
* SDL2_ttf
* SDL_sound (latest hg, apply provided patches!)
* vorbisfile
* pixman
* zlib (only ruby bindings)
* OpenGL header (alternatively GLES2 with `DEFINES+=GLES2_HEADER`)
* libiconv (on Windows, optional with INI_ENCODING)
* libguess (optional with INI_ENCODING)

mkxp employs Qt's qmake build system, so you'll need to install that beforehand. Alternatively, you can build with cmake (FIXME: add cmake instructions).

qmake will use pkg-config to locate the respective include/library paths. If you installed any dependencies into non-standard prefixes, make sure to adjust your `PKG_CONFIG_PATH` variable accordingly.

The exception is boost, which is weird in that it still hasn't managed to pull off pkg-config support (seriously?). *If you installed boost in a non-standard prefix*, you will need to pass its include path via `BOOST_I` and library path via `BOOST_L`, either as direct arguments to qmake (`qmake BOOST_I="/usr/include" ...`) or via environment variables. You can specify a library suffix (eg. "-mt") via `BOOST_LIB_SUFFIX` if needed.

Midi support is enabled by default and requires fluidsynth to be present at runtime (not needed for building); if mkxp can't find it at runtime, midi playback is disabled. It looks for `libfluidsynth.so.1` on Linux, `libfluidsynth.dylib.1` on OSX and `fluidsynth.dll` on Windows, so make sure to have one of these in your link path. If you still need fluidsynth to be hard linked at buildtime, use `CONFIG+=SHARED_FLUID`. When building fluidsynth yourself, you can disable almost all options (audio drivers etc.) as they are not used. Note that upstream fluidsynth has support for sharing soundfont data between synthesizers (mkxp uses multiple synths), so if your memory usage is very high, you might want to try compiling fluidsynth from git master.

By default, mkxp switches into the directory where its binary is contained and then starts reading the configuration and resolving relative paths. In case this is undesired (eg. when the binary is to be installed to a system global, read-only location), it can be turned off by adding `DEFINES+=WORKDIR_CURRENT` to qmake's arguments.

To auto detect the encoding of the game title in `Game.ini` and auto convert it to UTF-8, build with `CONFIG+=INI_ENCODING`. Requires iconv implementation and libguess. If the encoding is wrongly detected, you can set the "titleLanguage" hint in mkxp.conf.

**MRI-Binding**: pkg-config will look for `ruby-2.1.pc`, but you can modify mkxp.pro to use 2.0 instead. This is the default binding, so no arguments to qmake needed (`BINDING=MRI` to be explicit).

**MRuby-Binding**: place the "mruby" folder into the project folder and build it first. Add `BINDING=MRUBY` to qmake's arguments.

**Null-Binding**: Add `BINDING=NULL` to qmake's arguments.

### Supported image/audio formats
These depend on the SDL auxiliary libraries. For maximum RGSS compliance, build SDL2_image with png/jpg support, and SDL_sound with oggvorbis/wav/mp3 support.

To run mkxp, you should have a graphics card capable of at least **OpenGL (ES) 2.0** with an up-to-date driver installed.

## Configuration

mkxp reads configuration data from the file "mkxp.conf". The format is ini-style. Do *not* use quotes around file paths (spaces won't break). Lines starting with '#' are comments. See 'mkxp.conf.sample' for a list of accepted entries.

All option entries can alternatively be specified as command line options. Any options that are not arrays (eg. RTP paths) specified as command line options will override entries in mkxp.conf. Note that you will have to wrap values containing spaces in quotes (unlike in mkxp.conf).

The syntax is: `--<option>=<value>`

Example: `./mkxp --gameFolder="my game" --vsync=true --fixedFramerate=60`

## Midi music (*ALPHA STATUS*)

mkxp doesn't come with a soundfont by default, so you will have to supply it yourself (set its path in the config). Playback has been tested and should work reasonably well with all RTP assets.

Known issues with midi playback:

* Some songs' instruments become mute after looping

## Fonts

In the RMXP version of RGSS, fonts are loaded directly from system specific search paths (meaning they must be installed to be available to games). Because this whole thing is a giant platform-dependent headache, I decided to implement the behavior Enterbrain thankfully added in VX Ace: loading fonts will automatically search a folder called "Fonts", which obeys the default searchpath behavior (ie. it can be located directly in the game folder, or an RTP).

If a requested font is not found, no error is generated. Instead, a built-in font is used (currently "Liberation Sans").

## What doesn't work (yet)

* wma audio files
* The Win32API ruby class (for obvious reasons)
* Creating Bitmaps with sizes greater than the OpenGL texture size limit (around 8192 on modern cards)*

\* There is an exception to this, called *mega surface*. When a Bitmap bigger than the texture limit is created from a file, it is not stored in VRAM, but regular RAM. Its sole purpose is to be used as a tileset bitmap. Any other operation to it (besides blitting to a regular Bitmap) will result in an error.

## Nonstandard RGSS extensions

To alleviate possible porting of heavily Win32API reliant scripts, I have added certain functionality that you won't find in the RGSS spec. Currently this amounts to the following:

* The `Input.press?` family of functions accepts three additional button constants: `::MOUSELEFT`, `::MOUSEMIDDLE` and `::MOUSERIGHT` for the respective mouse buttons.
* The `Input` module has two additional functions, `#mouse_x` and `#mouse_y` to query the mouse pointer position relative to the game screen.
* The `Graphics` module has two additional properties: `fullscreen` represents the current fullscreen mode (`true` = fullscreen, `false` = windowed), `show_cursor` hides the system cursor inside the game window when `false`.
