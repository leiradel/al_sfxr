# al_sfxr GUI

This little GUI application is pretty much the same as the
[original](http://drpetter.se/project_sfxr.html). Notable differences are:

* **Back**: Undoes the last modification.
* **Copy**: Copies the `al_sfxr` code to recreate the current sound in runtime
  to the clipboard. This button is active only if the generator buttons were
  used to create the sound. If any of the manual settings have been changed,
  it's not possible to recreate the sound. In that case, export the WAV file to
  use in runtime.
* Should build and work on Windows, Linux, and OSX (only tested on Linux).

## Notes

* Heavily based on [sfxr-sdl](http://drpetter.se/files/sfxr-sdl-1.2.1.tar.gz)
* Font taken from [EightBit-Atari-Fonts](https://github.com/TheRobotFactory/EightBit-Atari-Fonts/blob/master/Original%20Files/PNG/90.png)

## License

The MIT License (MIT)

* Copyright (c) 2007 Tomas Pettersson (original implementation)
* Copyright (c) 2020 Andre Leiradella (updates)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
