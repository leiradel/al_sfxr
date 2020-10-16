# al_sfxr

**al_sfxr** is a header-only re-implementation of the original
[sfxr](https://www.drpetter.se/project_sfxr.html) sound effect generator.

Instead of exporting wave files, **al_sfxr** generates audio frames during
runtime directly from **sfxr** parameters, so it can be used to create sound
effects on-the-fly for host applications.

The provided GUI application can be used to play with the parameters and
export wave files, or to generate sound effects with a known seed that can be
used in runtime to generate the same effect without the need of a wafe file.

## Instructions

Define `AL_SFXR_IMPLEMENTATION` and include this file in one C or C++ file to
create the implementation. If you need to declare the functions somewhere
else, include this file without defining `AL_SFXR_IMPLEMENTATION`; the same
configuration used in the implementation must be used.

To configure **as_sfxr**, define one or more of these macros before including
this file:

* `AL_SFXR_GENERATE`: this macro enables `al_sfxr_generate`, which randomly
  generates a SFXR from a preset, a mutation count, and a seed.
* `AL_SFXR_LOAD`: define this macro to enable `al_sfxr_load`, used to load a
  SFXR file from an user-supplied reader function.
* `AL_SFXR_SAVE`: enables `al_sfxr_save` that writes the SFXR to an
  user-supplied writer function.
* `AL_SFXR_INT16_MONO`, `AL_SFXR_FLOAT_MONO`, `AL_SFXR_INT16_STEREO`, and
  `AL_SFXR_FLOAT_STEREO`: defines these macros to enable functions that will
  produce audio frames for a SFXR:
    * `al_sfxr_produce1i`: 44100 Hz, signed 16-bit mono
    * `al_sfxr_produce2i`: 44100 Hz, signed 16-bit stereo
    * `al_sfxr_produce1f`: 44100 Hz, 32-bit float mono
    * `al_sfxr_produce2f`: 44100 Hz, 32-bit float stereo

The API is fully documented in the `al_sfxr.h` file.

## TL;DR

```cpp
// Create a SFXR sound using the `AL_SFXR_LASER` preset, zero mutations, and 17
// as the pseudo-random number generator seed
al_sfxr_Params params;
al_sfxr_generate(&params, AL_SFXR_LASER, 0, 17);

// Save it
FILE* const fp = fopen("sfxr.bin", "wb");

if (al_sfxr_save(&params, fp_writer, (void*)fp) != 0) {
    // error
}

// Load a SFXR sound
al_sfxr_Params params;
FILE* const fp = fopen("sfxr.bin", "rb");

if (al_sfxr_load(&params, fp_reader, (void*)fp) != 0) {
    // error
}

// Play the sound
al_sfxr_Decoder decoder;
al_sfxr_start(&decoder, &params, 19); // see also al_sfxr_start_quick

while (1) {
    int16_t samples[1024 * 2]; // stereo

    // Produce 44100 Hz stereo signed 16-bit frames
    size_t const frames_written = al_sfxr_produce2i(&decoder, samples, 1024);
    size_t const samples_written = frames_written * 2;

    // Mix samples into your signed 16-bit 44100 Hz interleaved stereo output
    // sound buffer
    mix(samples, samples_written);
}
```

## License

The MIT License (MIT)

* Copyright (c) 2007 Tomas Pettersson (original implementation)
* Copyright (c) 2020 Andre Leiradella (header only library)

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
