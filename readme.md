# TitTat

This is a small toy I wrote to experiment with WASM.

The code in this repo is under the MIT License. raylib (linked as a git submodule) is covered under it's own permissive license.

Sounds were generated with [jsfxr](https://sfxr.me/).

## Dependencies

### Libraries

Tittat uses the fantastic [raylib](https://www.raylib.com/). It's linked as a git submodule and can be retrieved using:
`git submodule update --init --recursive`

### Tools

To build you'll need [CMake](https://cmake.org/), [MSVC](https://visualstudio.microsoft.com/) (only when building for Windows), and [emscripten](https://emscripten.org/) (when building for web).

I use the simple [http-server](https://github.com/http-party/http-server) when testing the web build.

## Web Shell
There is a minimal HTML file needed to display the game and prevent accidental scrolling in a browser here: [shell.html](source/shell.html).

## Supported Platforms
I've only tested on win32 and web platforms. Everything[^1] is written to be portable, and porting to other platforms should be straight forward[^2]. The [raylib documentation](https://github.com/raysan5/raylib/wiki) has some good info on building for different platforms.

## Building

Once you've got raylib by updating submodules and install the necessary tools (see above) you should be able to use one of the `build_*.bat` files to configure, build, and run Tittat. For example:

`build_win32 debug configure`
`build_win32 debug build`
`build_win32 debug run`

`build_web.bat` assumes the environment has been setup for emscripten with `emsdk_env.bat` or something equivalent. `build_web BUILD_TYPE run` tries to run `http-server` rooting in the build targets build directory.

`build_win32.bat` assumes the environment has been [setup to use MSVC](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170).

[^1]: Except the two short build_*.bat files.
[^2]: Famous last words.

