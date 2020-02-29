# mkxp-z

![ss](/screenshot.png?raw=true)

This is a fork of mkxp intended to be a little more than just a barebones recreation of RPG Maker. The original goal was successfully running games based on Pokemon Essentials, which is notoriously dependent on Windows APIs. I'd consider that mission accomplished.

## Bindings
Bindings provide the glue code for an interpreted language environment to run game scripts in. mkxp-z focuses on MRI and as such the mruby and null bindings are not included. The original MRI bindings remain. Please see the original README for more details.

### MRI
Website: https://www.ruby-lang.org/en/

Matz's Ruby Interpreter, also called CRuby, is the most widely deployed version of ruby. MRI 1.8.1 is what was used in RPG Maker XP, and 1.8.7 is the lowest that mkxp-z is prepared to let you go. 1.8.1 and 1.8.7 are for the most part identical, though there are a few differences that are ironed out in my fork of 1.8.7.

Ruby versions 2.1 - 2.7 are also supported (and recommended for the capable).

This binding should support RGSS1, RGSS2 and RGSS3. Note that I've only tested the Ruby 1.8.7 bindings with RGSS1.

## Dependencies / Building

* [ObjFW](https://github.com/ObjFW/ObjFW)
* libsigc++ 2.0
* PhysFS (latest hg)
* OpenAL
* SDL2*
* SDL2_image
* SDL2_ttf
* [Ancurio's SDL_sound fork](https://github.com/Ancurio/SDL_sound)
* [My Ruby 1.8 fork](https://github.com/inori-z/ruby/tree/ruby_1_8_7), if using Ruby 1.8
* vorbisfile
* pixman
* zlib
* OpenGL header

(* For the F1 menu to work correctly under Linux/X11, you need latest hg + [this patch](https://bugzilla.libsdl.org/show_bug.cgi?id=2745))

mkxp-z employs the meson build system, so you'll need to install that beforehand.

meson will use pkg-config to locate the respective include/library paths. If you installed any dependencies into non-standard prefixes, make sure to set `-Dpkg_config_path` accordingly when configuring the build. If pkgconfig cannot find a dependency, meson will attempt to use CMake scripts instead (if CMake is installed), followed by system installations/macOS frameworks.

Midi support is enabled by default and requires fluidsynth to be present at runtime (not needed for building); if mkxp can't find it at runtime, midi playback is disabled. It looks for `libfluidsynth.so.1` on Linux, `libfluidsynth.dylib.1` on OSX and `fluidsynth.dll` on Windows, so make sure to have one of these in your link path. If you still need fluidsynth to be hard linked at buildtime, use `-Dshared_fluid=true`. When building fluidsynth yourself, you can disable almost all options (audio drivers etc.) as they are not used. Note that upstream fluidsynth has support for sharing soundfont data between synthesizers (mkxp uses multiple synths), so if your memory usage is very high, you might want to try compiling fluidsynth from git master.

By default, mkxp switches into the directory where its binary is contained and then starts reading the configuration and resolving relative paths. In case this is undesired (eg. when the binary is to be installed to a system global, read-only location), it can be turned off by adding `-Dworkdir_current=true` to meson's build arguments.

**MRI-Binding**: Meson will use pkg-config to look for `ruby-X.Y.pc`, where `X` is the major version number and `Y` is the minor version number (e.g. `ruby-2.5.pc`). The version that will be searched for can be set with `-Dmri_version=X.Y`. `mri-version` is set to `1.8` by default.

## Supported image/audio formats

These depend on the SDL auxiliary libraries. For maximum RGSS compliance, build SDL2_image with png/jpg support, and SDL_sound with oggvorbis/wav/mp3 support.

To run mkxp, you should have a graphics card capable of at least **OpenGL 4.1** (or 3.3 with openGL4 set to false in mkxp.json) with an up-to-date driver installed.

## Platform-specific scripts

By including `|p|` or `|!p|` at the very beginning of a script's title, you can specify what platform the script should or shouldn't run on. `p` should match the first letter of your platform, and is case-insensitive. As an example, a script named `|!W| Socket` would not run on Windows, while a script named `|W| Socket` would *only* run on Windows.

## Midi music

mkxp doesn't come with a soundfont by default, so you will have to supply it yourself (set its path in the config). Playback has been tested and should work reasonably well with all RTP assets.

You can use this public domain soundfont: [GMGSx.sf2](https://www.dropbox.com/s/qxdvoxxcexsvn43/GMGSx.sf2?dl=0)

## Fonts

In the RMXP version of RGSS, fonts are loaded directly from system specific search paths (meaning they must be installed to be available to games). Because this whole thing is a giant platform-dependent headache, Ancurio decided to implement the behavior Enterbrain thankfully added in VX Ace: loading fonts will automatically search a folder called "Fonts", which obeys the default searchpath behavior (ie. it can be located directly in the game folder, or an RTP).

If a requested font is not found, no error is generated. Instead, a built-in font is used. By default, this font is Liberation Sans. WenQuanYi MicroHei is used as the built-in font if the `cjk_fallback_font` option is used.

## What doesn't work (yet)
* Movie playback
* wma audio files
* Creating Bitmaps with sizes greater than the OpenGL texture size limit (around 16384 on modern cards)^

^ There is an exception to this, called *mega surface*. When a Bitmap bigger than the texture limit is created from a file, it is not stored in VRAM, but regular RAM. Its sole purpose is to be used as a tileset bitmap. Any other operation to it (besides blitting to a regular Bitmap) will result in an error.
