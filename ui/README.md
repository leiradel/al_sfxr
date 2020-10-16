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
* Released under the MIT license
