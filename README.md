# mkxp-z

This is a work-in-progress fork of mkxp that is intended to run as similarly to RPG Maker XP (RGSS1) as possible, specifically with the target of running games based on Pokemon Essentials, ideally without having to change a single line of code. Once this goal can be accomplished, it's possible that optional enhancements (such as Discord integration) can be written for fangame developers (you poor souls) to take advantage of.

## Prebuilt binaries
> None yet!

## Bindings
Bindings provide the glue code for an interpreted language environment to run game scripts in. mkxp-z focuses on Ruby 1.8 and as such the mruby and null bindings are not included. The original MRI bindings remain for the time being.. Please see the original README for more details.

### MRI
Website: https://www.ruby-lang.org/en/

Matz's Ruby Interpreter, also called CRuby, is the most widely deployed version of ruby. If you're interested in running games created with RPG Maker XP, this is the one you should go for. MRI 1.8 is what was used in RPG Maker XP, however, this binding is written against 2.0 (the latest version). For games utilizing only the default scripts provided by Enterbrain, this binding works quite well so far. Note that there are language and syntax differences between 1.8 and 2.0, so some user created scripts may not work correctly.

For a list of differences, see:
http://stackoverflow.com/questions/21574/what-is-the-difference-between-ruby-1-8-and-ruby-1-9

This binding supports RGSS1, RGSS2 and RGSS3.

> Note: Experimental support for Ruby 1.8's API has been added. 

## Dependencies / Building

* Boost.Unordered (headers only)
* Boost.Program_options
* libsigc++ 2.0
* PhysFS (latest hg)
* OpenAL
* SDL2*
* SDL2_image
* SDL2_ttf
* [Ancurio's SDL_sound fork](https://github.com/Ancurio/SDL_sound)
* [My Ruby 1.8 fork](https://github.com/inori-z/ruby/tree/ruby_1_8_7), for the Zlib class and an actual working Windows build
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

### Supported image/audio formats
These depend on the SDL auxiliary libraries. For maximum RGSS compliance, build SDL2_image with png/jpg support, and SDL_sound with oggvorbis/wav/mp3 support.

To run mkxp, you should have a graphics card capable of at least **OpenGL (ES) 2.0** with an up-to-date driver installed.

## Platform-specific build instructions

This section still needs to be written, but to start with, use Homebrew to obtain packages on macOS and MSYS2+pacman to obtain them on Windows. For Ubuntu, refer to the dockerfile in the Docker directory.

## Configuration

mkxp reads configuration data from the file "mkxp.conf". The format is ini-style. Do *not* use quotes around file paths (spaces won't break). Lines starting with '#' are comments. See 'mkxp.conf.sample' for a list of accepted entries.

All option entries can alternatively be specified as command line options. Any options that are not arrays (eg. RTP paths) specified as command line options will override entries in mkxp.conf. Note that you will have to wrap values containing spaces in quotes (unlike in mkxp.conf).

The syntax is: `--<option>=<value>`

Example: `./mkxp --gameFolder="my game" --vsync=true --fixedFramerate=60`

## Midi music

mkxp doesn't come with a soundfont by default, so you will have to supply it yourself (set its path in the config). Playback has been tested and should work reasonably well with all RTP assets.

You can use this public domain soundfont: [GMGSx.sf2](https://www.dropbox.com/s/qxdvoxxcexsvn43/GMGSx.sf2?dl=0)

## Fonts

In the RMXP version of RGSS, fonts are loaded directly from system specific search paths (meaning they must be installed to be available to games). Because this whole thing is a giant platform-dependent headache, Ancurio decided to implement the behavior Enterbrain thankfully added in VX Ace: loading fonts will automatically search a folder called "Fonts", which obeys the default searchpath behavior (ie. it can be located directly in the game folder, or an RTP).

If a requested font is not found, no error is generated. Instead, a built-in font is used (currently "Liberation Sans").

## What doesn't work (yet)

* Win32API calls outside of Windows (Win32API is just an alias to the MiniFFI class, which *does* work with other operating systems, but you can obviously only load libraries made for the platform you're on)*
* Some Win32API calls don't play nicely with SDL. Building with the `fix_essentials` option will attempt to fix this.
* Sockets in Essentials.
* Movie playback
* wma audio files
* Creating Bitmaps with sizes greater than the OpenGL texture size limit (around 8192 on modern cards)^

\* Once games can be played comfortably on Windows, I may try to have a 'fake' Win32API class written for other operating systems which intercepts and interprets some of the common calls that get used, a bit like what's already being done with the `fix_essentials` option already)

^ There is an exception to this, called *mega surface*. When a Bitmap bigger than the texture limit is created from a file, it is not stored in VRAM, but regular RAM. Its sole purpose is to be used as a tileset bitmap. Any other operation to it (besides blitting to a regular Bitmap) will result in an error. (This breaks SLLD after the professor's speech due to an issue with the tilemaps, but Pokemon Uranium seems to be okay)

## Nonstandard RGSS extensions

To alleviate possible porting of heavily Win32API reliant scripts, Ancurio added certain functionality that you won't find in the RGSS spec. Currently this amounts to the following:

* The `Input.press?` family of functions accepts three additional button constants: `::MOUSELEFT`, `::MOUSEMIDDLE` and `::MOUSERIGHT` for the respective mouse buttons.
* The `Input` module has two additional functions, `#mouse_x` and `#mouse_y` to query the mouse pointer position relative to the game screen.
* The `Graphics` module has two additional properties: `fullscreen` represents the current fullscreen mode (`true` = fullscreen, `false` = windowed), `show_cursor` hides the system cursor inside the game window when `false`.
