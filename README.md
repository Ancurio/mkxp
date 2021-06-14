# mkxp-z

<p align=center>
    <img src="screenshot.png?raw=true" width=512 height=412>
</p>

This is a fork of mkxp intended to be a little more than just a barebones recreation of RPG Maker. The original goal was successfully running games based on Pokemon Essentials, which is notoriously dependent on Windows APIs. I'd consider that mission accomplished.

Despite the fact that it was made with Essentials games in mind, there is nothing connected to it contained in this repository, and it should still be compatible with anything that runs in the upstream version of MKXP. You can think of it as MKXP but a bit supercharged --  it should be able to run all but the most demanding of RGSS projects, given a bit of porting work.

It supports Windows, Linux and both Intel and Apple Silicon versions of macOS.

Releases are [here](https://gitlab.com/mkxp-z/mkxp-z/-/releases). Requirements for running them are Windows 8.1+, Ubuntu 18.04+ (Fedora and Manjaro releases that age or newer *should* also be fine), or macOS 10.12+.

I'd highly recommend [checking the gitbook](https://roza-gb.gitbook.io/mkxp-z) for more information than this readme contains.

## Bindings
Bindings provide the glue code for an interpreted language environment to run game scripts in. mkxp-z focuses on MRI and as such the mruby and null bindings are not included.

### MRI
Website: https://www.ruby-lang.org/en/

Matz's Ruby Interpreter, also called CRuby, is the most widely deployed version of ruby. MRI 1.8.1 is what was used in RPG Maker XP, and 1.8.7 is the lowest that mkxp-z is prepared to let you go.

Ruby versions 1.9.3 and 2.1 - 3.0 are also supported, and running each platform's respective dependency makefile will build Ruby 3.

## Dependencies / Building

For more detailed build instructions, refer to the linked gitbook.

Firstly, each platform has a set of tools and libraries that must be installed prior to building anything:

+ **macOS (through Homebrew)**

```sh
brew install libtool cmake automake autoconf pkg-config
```

+ **Windows (MSYS 64-Bit)**

```sh
# Assuming 64-bit
pacman -S git ruby base-devel mingw-w64-x86_64-cmake mingw-w64-x86_64-meson mingw-w64-x86_64-gcc
```

+ **Linux (Ubuntu/Debian)**

```sh
sudo apt install git build-essential cmake meson autoconf automake libtool pkg-config ruby bison zlib1g-dev xorg-dev lib32z1 libasound2-dev libpulse-dev
```

If Meson complains about not being able to find OpenGL, you also need `libgl1-mesa-dev`. After having all the prerequisites, go to your platform's respective folder and run `make` (or `setup.command` on macOS).

Following that, you should be able to just build mkxp-z:

### macOS

After having all the prerequisites, go to the `macos` directory and run `setup.command`. This will build all the remaining dependencies you need.

Following that, open the Xcode project, select the scheme you want (Universal and Apple Silicon options currently don't work with Intel Macs), and build.

### Windows/Linux

After having all the prerequisites, go to the `windows` or `linux` directory and run `make`. This will build all the remaining dependencies you need.

`source linux/vars.sh` or `source windows/vars.sh` depending on your platform, then run `meson build` and `ninja -C build`.

By default, mkxp switches into the directory where its binary is contained and then starts reading the configuration and resolving relative paths. In case this is undesired (eg. when the binary is to be installed to a system global, read-only location), it can be turned off by adding `-Dworkdir_current=true` to meson's build arguments.

**MRI-Binding**: Meson will use pkg-config to look for `ruby-X.Y.pc`, where `X` is the major version number and `Y` is the minor version number (e.g. `ruby-3.0.pc`). The version that will be searched for can be set with `-Dmri_version=X.Y`. `mri-version` is set to `3.0` by default.

## Platform-specific scripts

By including `|p|` or `|!p|` at the very beginning of a script's title, you can specify what platform the script should or shouldn't run on. `p` should match the first letter of your platform, and is case-insensitive. As an example, a script named `|!W| Socket` would not run on Windows, while a script named `|W| Socket` would *only* run on Windows.

## Midi music

mkxp doesn't come with a soundfont by default, so you will have to supply it yourself (set its path in the config). Playback has been tested and should work reasonably well with all RTP assets.

You can use this public domain soundfont: [GMGSx.sf2](https://www.dropbox.com/s/qxdvoxxcexsvn43/GMGSx.sf2?dl=0)

## macOS Controller Support

Binding controller buttons on macOS is slightly different depending on which version you are running. Binding specific buttons requires different versions of the operating system:

+ **Thumbstick Button (L3/R3, LS/RS, L↓/R↓)**: macOS Mojave 10.14.1+
+ **Start/Select (Options/Share, Menu/Back, Plus/Minus)**: macOS Catalina 10.15+
+ **Home (Guide, PS)**: macOS Big Sur 11.0+

Technically, while SDL itself might support these buttons, the keybinding menu had to be rewritten in Cocoa in a hurry, as switching away from native OpenGL broke the original keybinding menu. (ANGLE is used instead, to prevent crashing on Apple Silicon releases of macOS, and to help mkxp switch to Metal in the future)

## Fonts

In the RMXP version of RGSS, fonts are loaded directly from system specific search paths (meaning they must be installed to be available to games). Because this whole thing is a giant platform-dependent headache, Ancurio decided to implement the behavior Enterbrain thankfully added in VX Ace: loading fonts will automatically search a folder called "Fonts", which obeys the default searchpath behavior (ie. it can be located directly in the game folder, or an RTP).

If a requested font is not found, no error is generated. Instead, a built-in font is used. By default, this font is Liberation Sans. WenQuanYi MicroHei is used as the built-in font if the `cjk_fallback_font` option is used.

## What doesn't work (yet)
* Movie playback
* wma audio files
* Creating Bitmaps with sizes greater than the OpenGL texture size limit (around 16384 on modern cards).^

^ There is an exception to this, called *mega surface*. When a Bitmap bigger than the texture limit is created from a file, it is not stored in VRAM, but regular RAM. Its sole purpose is to be used as a tileset bitmap. Any other operation to it (besides blitting to a regular Bitmap) will result in an error.
