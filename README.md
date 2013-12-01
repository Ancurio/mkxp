# mkxp

mkxp is a project that seeks to provide a fully open source implementation of the Ruby Game Scripting System (RGSS) interface used in the popular game creation software "RPG Maker XP" (trademark by Enterbrain, Inc.), with focus on Linux. The goal is to be able to run games created with the above software natively without changing a single file.

It is licensed under the GNU General Public License v2+.

## Bindings
Bindings provide the glue code for an interpreted language environment to run game scripts in. As of right now, they are compiled directly into the executable (however, the scripting language itself might not be). Currently there are three bindings:

### MRI
Website: https://www.ruby-lang.org/en/

Matz's Ruby Interpreter, also called CRuby, is the most widely deployed version of ruby. If you're interested in running games created with RPG Maker XP, this is the one you should go for. MRI 1.8 is what was used in RPG Maker XP, however, this binding is written against 2.0 (the latest version). For games utilizing only the default scripts provided by Enterbrain, this binding works quite well so far. Note that there are language and syntax differences between 1.8 and 2.0, so some user created scripts may not work correctly.

For a list of differences, see:
http://stackoverflow.com/questions/21574/what-is-the-difference-between-ruby-1-8-and-ruby-1-9

To select this binding, run `qmake BINDING=BINDING_MRI`

### mruby (Lightweight Ruby)
Website: https://github.com/mruby/mruby

mruby is a new endeavor by Matz and others to create a more lightweight, spec-adhering, embeddable Ruby implementation. You can think of it as a Ruby version of Lua.

Due to heavy differences between mruby and MRI as well as lacking modules, running RPG Maker games with this binding will most likely not work correctly. It is provided as experimental code. You can eg. write your own ruby scripts and run them.

Some extensions to the standard classes/modules are provided taking the RPG Maker XP helpfile as a quasi "standard". These include Marshal, File, FileTest and Time.

**Important:** If you decide to use [mattn's oniguruma regexp gem](https://github.com/mattn/mruby-onig-regexp), don't forget to add `-lonig` to the linker flags to avoid ugly symbol overlaps with libc.

To select this binding, run `qmake BINDING=BINDING_MRUBY`

### null
This binding only exists for testing purposes and does nothing (the engine quits immediately). It can be used to eg. run a minimal RGSS game loop directly in C++.

To select this binding, run `qmake BINDING=BINDING_NULL`

## Dependencies

* QtCore 4.8
* libsigc++
* PhysFS
* glew
* OpenAL
* SDL2
* SDL2_image
* SDL2_ttf
* SDL_sound
* pixman
* zlib (only ruby bindings)

(If no version specified, assume latest *development version*, ie. freshest one from git/hg/svn)

### Supported image/audio formats
These depend on the auxiliary libraries. For maximum RGSS compliance, build SDL2_image with png/jpg support, and SDL_sound with oggvorbis/wav/mp3 support.
 
### MRI binding:
Place a recent version of ruby in the project folder, apply all patches from "patches/ruby" and build it.

### mruby binding:
Place a recent version of mruby in the project folder and build it.

To run mkxp, you should have a graphics card capable of at least **OpenGL 2.0** with an up-to-date driver installed.

## Building

mkxp employs Qt's qmake build system, so you'll need to install that beforehand. After cloning mkxp, run one of the above qmake calls, or simply `qmake` to select the default binding (currently MRI), then `make`.

## Configuration

mkxp reads configuration data from the file "mkxp.conf" contained in the current directory. The format is ini-style. The "[General]" group may contain following entries:

| Key              | Type        | Default | Description                                                                     |
| ---------------- | ----------- | ------- | ------------------------------------------------------------------------------- |
| debugMode        | bool        | false   | Log OpenGL debug information to the console                                     |
| winResizable     | bool        | false   | Game window is resizable                                                        |
| fullscreen       | bool        | false   | Start game in fullscreen (this can always be toggled with Alt-Enter at runtime) |
| fixedAspectRatio | bool        | true    | Don't stretch the game screen to fit the window size                            |
| smoothScaling    | bool        | false   | Apply linear interpolation when game screen is stretched                        |
| vsync            | bool        | false   | Sync screen redraws to the monitor refresh rate                                 |
| defScreenW       | int         | 640     | Width the game window starts in (this is **not** the game resolution)           |
| defScreenH       | int         | 480     | Height the game window starts in                                                |
| fixedFramerate   | int         | 0       | FPS will be fixed to this amount. Ignored if 0.                                 |
| solidFonts       | bool        | false   | Don't use alpha blending for fonts                                              |
| fastSoundPitch   | bool        | true    | Use cheap OpenAL pitch shifting for sounds (SE)                                 |
| fastMusicPitch   | bool        | false   | Use cheap OpenAL pitch shifting for music (BGM/BGS/ME)                          |
| gameFolder       | string      | "."     | mkxp will look for all game related files here                                  |
| allowSymlinks    | bool        | false   | Allow symlinks to be followed in the game folder.                               |
| customScript     | string      | ""      | Execute a raw ruby script file instead of an RPG Maker game.                    |
| RTPs             | string list | ""      | A list of space separated paths to RTPs to be used (See next section)           |

## RTPs

As of right now, mkxp doesn't support midi files, so to use the default RTPs provided by Enterbrain you will have to convert all midi tracks (those in BGM and ME) to ogg or wav. Make sure that the file names match up, ie. "foobar.mid" should be converted to "foobar.ogg".

## Fonts

In the RMXP version of RGSS, fonts are loaded directly from system specific search paths (meaning they must be installed to be available to games). Because this whole thing is a giant platform-dependent headache, I decided to implement the behavior Enterbrain thankfully added in VX Ace: loading fonts will automatically search a folder called "Fonts", which obeys the default searchpath behavior (ie. it can be located directly in the game folder, or an RTP).

If a requested font is not found, no error is generated. Instead, a built-in font is used (currently "Liberation Sans").

## What doesn't work (yet)

* midi and wma audio files
* Audio "pitch" parameter (OpenAL offers a cheap alternative by playing sound faster/slower)
* The Win32API ruby class (for obvious reasons)
* Restarting the game with F12
* Creating Bitmaps with sizes greater than the OpenGL texture size limit (around 8192 on modern cards)*

\* There is an exception to this, called *mega surface*. When a Bitmap bigger than the texture limit is created from a file, it is not stored in VRAM, but regular RAM. Its sole purpose is to be used as a tileset bitmap. Any other operation to it (besides blitting to a regular Bitmap) will result in an error.

## Nonstandard RGSS extensions

To alleviate possible porting of heavily Win32API reliant scripts, I have added certain functionality that you won't find in the RGSS spec. Currently this amounts to the following:

* The `Input.press?` family of functions accepts three additional button constants: `::MOUSELEFT`, `::MOUSEMIDDLE` and `::MOUSERIGHT` for the respective mouse buttons.
* The `Input` module has two additional functions, `#mouse_x` and `#mouse_y` to query the mouse pointer position relative to the game window.
* The `Graphics` module has two additional properties: `fullscreen` represents the current fullscreen mode (`true` = fullscreen, `false` = windowed), `show_cursor` hides the system cursor inside the game window when `false`.
