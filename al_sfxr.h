#define AL_SFXR_GENERATE
#define AL_SFXR_LOAD
#define AL_SFXR_SAVE
#define AL_SFXR_INT16_MONO
#define AL_SFXR_FLOAT_MONO
#define AL_SFXR_INT16_STEREO
#define AL_SFXR_FLOAT_STEREO
#define AL_SFXR_IMPLEMENTATION

/*
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
  generates a SFXR from a preset and a seed.
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

## Sample code

```cpp
// Create a SFXR sound using the `AL_SFXR_LASER` preset and 17 as the
// pseudo-random number generator seed
al_sfxr_Params params;
al_sfxr_generate(&params, AL_SFXR_LASER, 17);

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

Copyright (c) 2007 Tomas Pettersson (original implementation)
Copyright (c) 2020 Andre Leiradella (header only library)

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
*/

#ifndef AL_SFXR_H
#define AL_SFXR_H

#include <stdint.h>
#include <stddef.h>

/**
 * The type of the wave to generate.
 */
typedef enum {
    AL_SFXR_SQUARE,
    AL_SFXR_SAWTOOTH,
    AL_SFXR_SINEWAVE,
    AL_SFXR_NOISE
}
al_sfxr_Wave;

/**
 * The parameters to create a SFXR decoder.
 */
typedef struct {
    al_sfxr_Wave wave_type;

    float p_base_freq;
    float p_freq_limit;
    float p_freq_ramp;
    float p_freq_dramp;
    float p_duty;
    float p_duty_ramp;

    float p_vib_strength;
    float p_vib_speed;

    float p_env_attack;
    float p_env_sustain;
    float p_env_decay;
    float p_env_punch;

    float p_lpf_resonance;
    float p_lpf_freq;
    float p_lpf_ramp;
    float p_hpf_freq;
    float p_hpf_ramp;

    float p_pha_offset;
    float p_pha_ramp;

    float p_repeat_speed;

    float p_arp_speed;
    float p_arp_mod;

    float sound_vol;
}
al_sfxr_Params;

/**
 * A Newlib LCG pseudo-random number generator.
 *
 * @link http://en.wikipedia.org/wiki/Linear_congruential_generator
 */
typedef struct {
    uint64_t seed;
}
al_sfxr_Prng;

/**
 * A decoder to generate audio frames from SFXR parameters.
 */
typedef struct {
    al_sfxr_Params params;
    al_sfxr_Prng prng;

    int playing_sample;
    int phase;
    double fperiod;
    double fmaxperiod;
    double fslide;
    double fdslide;
    int period;
    float square_duty;
    float square_slide;
    int env_stage;
    int env_time;
    int env_length[3];
    float env_vol;
    float fphase;
    float fdphase;
    int iphase;
    float phaser_buffer[1024];
    int ipp;
    float noise_buffer[32];
    float fltp;
    float fltdp;
    float fltw;
    float fltw_d;
    float fltdmp;
    float fltphp;
    float flthp;
    float flthp_d;
    float vib_phase;
    float vib_speed;
    float vib_amp;
    int rep_time;
    int rep_limit;
    int arp_time;
    int arp_limit;
    double arp_mod;
}
al_sfxr_Decoder;

#if defined(AL_SFXR_GENERATE)
/**
 * Presets that can be used to generate a new SFXR sound with al_sfxr_generate.
 */
typedef enum {
    AL_SFXR_RANDOM,
    AL_SFXR_PICKUP,
    AL_SFXR_LASER,
    AL_SFXR_EXPLOSION,
    AL_SFXR_POWERUP,
    AL_SFXR_HIT,
    AL_SFXR_JUMP,
    AL_SFXR_BLIP
}
al_sfxr_Preset;

/**
 * Randomly generates a SFXR with the given preset, number of mutations, and
 * pseudo-random number generator seed. This function will always create the
 * same sound effect given the same preset, number of mutations, and PRNG seed.
 *
 * @param params where the SFXR will be generated
 * @param preset the preset to use in the generation
 * @param mutations the number of mutations to perform after generation
 * @param seed the seed for the pseudo-random number generator
 */
void al_sfxr_generate(al_sfxr_Params* const params, al_sfxr_Preset const preset, unsigned const mutations, uint64_t const seed);
#endif /* AL_SFXR_GENERATE */

#if defined(AL_SFXR_LOAD)
/**
 * Reads a byte in an user defined way into the last argument.
 *
 * @param userdata a pointer to the implementation specific data
 * @param byte where the byte will be read into
 *
 * @result 0 if successful, something else on error
 */
typedef int (*al_sfxr_Read8)(void* const userdata, uint8_t* const byte);

/**
 * Loads a SFXR from a reader. The reader will be called for each byte that
 * this function needs to read, and will receive the userdata that is passed
 * to this function.
 *
 * @param params where the SFXR will be loaded to
 * @param reader the function used to read bytes
 * @param userdata the userdata that will be passed to the reader
 *
 * @return 0 if successful, something else on error
 *
 * @see al_sfxr_save
 */
int al_sfxr_load(al_sfxr_Params* const params, al_sfxr_Read8 const reader, void* const userdata);
#endif /* AL_SFXR_LOAD */

#if defined(AL_SFXR_SAVE)
/**
 * Writes a byte in a user defined way.
 *
 * @param userdata a pointer to the implementation specific data
 * @param byte the byte to write
 *
 * @return 0 if successful, something else on error
 */
typedef int (*al_sfxr_Write8)(void* const userdata, uint8_t byte);

/**
 * Saves the SFXR to a writer. The writer will be called for each byte that
 * this function needs to write, and will receive the userdata that is passed
 * to this function. Saved data is platform-independent as long as the
 * platforms use IEEE 754 binary32 floating point numbers.
 *
 * @param params the SFXR to save
 * @param writer the function used to write bytes
 * @param userdata the userdata that will be passed to the writer
 *
 * @return 0 if successful, something else on error
 *
 * @see al_sfxr_load
 */
int al_sfxr_save(al_sfxr_Params const* const params, al_sfxr_Write8 const writer, void* const userdata);
#endif /* AL_SFXR_SAVE */

/**
 * Starts playing a SFXR. A playing SFXR is called a decoder, and there can be
 * many voices playing concurrently for the same SFXR. The parameters will be
 * copied into the decoder and can be disposed when the function returns. The
 * seed is used to initialize a PRNG to generate noise during play, if needed.
 *
 * @param decoder the playing decoder created by the function
 * @param params the SFXR to play
 * @param seed the seed for the PRNG
 *
 * @see al_sfxr_start_quick
 * @see al_sfxr_restart
 */
void al_sfxr_start(al_sfxr_Decoder* const decoder, al_sfxr_Params const* const params, uint64_t const seed);

/**
 * Starts playing a SFXR. The parameters will be copied into the decoder and
 * can be disposed when the function returns. It uses the Newlib LCG with a
 * hardcoded seed.
 *
 * @param decoder the playing decoder created by the function
 * @param params the SFXR to play
 *
 * @see al_sfxr_start
 * @see al_sfxr_restart
 */
void al_sfxr_start_quick(al_sfxr_Decoder* const decoder, al_sfxr_Params const* const params);

/**
 * Restarts a SFXR, leaving the decoder in the same state as it was when one
 * of the start functions was called to create the decoder.
 *
 * @param decoder the playing decoder created by the function
 * @param params the SFXR to play
 * @param seed the seed for the PRNG
 *
 * @see al_sfxr_start
 * @see al_sfxr_start_quick
 */
void al_sfxr_restart(al_sfxr_Decoder* const decoder);

#if defined(AL_SFXR_INT16_MONO)
/**
 * Produces num_frames of mono audio into the output buffer. The buffer must
 * have num_frames * 2 bytes available.
 *
 * @param decoder the decoder from which to generate the audio frames
 * @param frames the output buffer
 * @param num_frames the number of frames to write
 *
 * @result the number of frames written
 *
 * @see al_sfxr_produce2i
 * @see al_sfxr_produce1f
 * @see al_sfxr_produce2f
 */
size_t al_sfxr_produce1i(al_sfxr_Decoder* const decoder, int16_t* frames, size_t const num_frames);
#endif /* AL_SFXR_INT16_MONO */

#if defined(AL_SFXR_INT16_STEREO)
/**
 * Produces num_frames of stereo audio into the output buffer. The buffer must
 * have num_frames * 4 bytes available.
 *
 * @param decoder the decoder from which to generate the audio frames
 * @param frames the output buffer
 * @param num_frames the number of frames to write
 *
 * @result the number of frames written
 *
 * @see al_sfxr_produce1i
 * @see al_sfxr_produce1f
 * @see al_sfxr_produce2f
 */
size_t al_sfxr_produce2i(al_sfxr_Decoder* const decoder, int16_t* frames, size_t const num_frames);
#endif /* AL_SFXR_INT16_STEREO */

#if defined(AL_SFXR_FLOAT_MONO)
/**
 * Produces num_frames of mono audio into the output buffer. The buffer must
 * have num_frames * 4 bytes available.
 *
 * @param decoder the decoder from which to generate the audio frames
 * @param frames the output buffer
 * @param num_frames the number of frames to write
 *
 * @result the number of frames written
 *
 * @see al_sfxr_produce1i
 * @see al_sfxr_produce2i
 * @see al_sfxr_produce2f
 */
size_t al_sfxr_produce1f(al_sfxr_Decoder* const decoder, float* frames, size_t const num_frames);
#endif /* AL_SFXR_FLOAT_MONO */

#if defined(AL_SFXR_FLOAT_STEREO)
/**
 * Produces num_frames of stereo audio into the output buffer. The buffer must
 * have num_frames * 8 bytes available.
 *
 * @param decoder the decoder from which to generate the audio frames
 * @param frames the output buffer
 * @param num_frames the number of frames to write
 *
 * @result the number of frames written
 *
 * @see al_sfxr_produce1i
 * @see al_sfxr_produce2i
 * @see al_sfxr_produce1f
 */
size_t al_sfxr_produce2f(al_sfxr_Decoder* const decoder, float* frames, size_t const num_frames);
#endif /* AL_SFXR_FLOAT_STEREO */

#endif /* !AL_SFXR_H */

#if defined(AL_SFXR_IMPLEMENTATION)

#include <string.h>
#include <math.h>

static void al_sfxr_newprng(al_sfxr_Prng* const prng, uint64_t const seed) {
    prng->seed = seed + (seed == 0);
}

static uint32_t al_sfxr_randu(al_sfxr_Prng* const prng, uint32_t const max_m1) {
    if (max_m1 == UINT32_MAX) {
        prng->seed = UINT64_C(6364136223846793005) * prng->seed + 1;
        return (uint32_t)(prng->seed >> 32);
    }

    uint32_t const max = max_m1 + 1;
    uint64_t const num_fits = ((uint64_t)UINT32_MAX + 1) / max;
    uint64_t const max_rn = num_fits * max;

    while (1) {
        prng->seed = UINT64_C(6364136223846793005) * prng->seed + 1;
        uint32_t const rn = (uint32_t)(prng->seed >> 32);

        if (rn < max_rn) {
            return rn % max;
        }
    }
}

static float al_sfxr_randf(al_sfxr_Prng* const prng, float const max) {
    return (float)(al_sfxr_randu(prng, UINT32_MAX)) * max / (float)UINT32_MAX;
}

#if defined(AL_SFXR_LOAD) || defined(AL_SFXR_GENERATE)
static void al_sfxr_zero(al_sfxr_Params* const params) {
    params->wave_type = AL_SFXR_SQUARE;

    params->p_base_freq = 0.3f;
    params->p_freq_limit = 0.0f;
    params->p_freq_ramp = 0.0f;
    params->p_freq_dramp = 0.0f;
    params->p_duty = 0.0f;
    params->p_duty_ramp = 0.0f;

    params->p_vib_strength = 0.0f;
    params->p_vib_speed = 0.0f;

    params->p_env_attack = 0.0f;
    params->p_env_sustain = 0.3f;
    params->p_env_decay = 0.4f;
    params->p_env_punch = 0.0f;

    params->p_lpf_resonance = 0.0f;
    params->p_lpf_freq = 1.0f;
    params->p_lpf_ramp = 0.0f;
    params->p_hpf_freq = 0.0f;
    params->p_hpf_ramp = 0.0f;

    params->p_pha_offset = 0.0f;
    params->p_pha_ramp = 0.0f;

    params->p_repeat_speed = 0.0f;

    params->p_arp_speed = 0.0f;
    params->p_arp_mod = 0.0f;

	params->sound_vol = 0.5f;
}
#endif /* defined(AL_SFXR_LOAD) || defined(AL_SFXR_GENERATE) */

#if defined(AL_SFXR_LOAD)
static int al_sfxr_read16(al_sfxr_Read8 const reader, void* const userdata, uint16_t* const word) {
    uint8_t l, h;

    int ok = reader(userdata, &l);
    ok = ok || reader(userdata, &h);

    *word = (uint16_t)h << 8 | l;
    return ok;
}

static int al_sfxr_read32(al_sfxr_Read8 const reader, void* const userdata, uint32_t* const dword) {
    uint16_t l, h;

    int ok = al_sfxr_read16(reader, userdata, &l);
    ok = ok || al_sfxr_read16(reader, userdata, &h);

    *dword = (uint32_t)h << 16 | l;
    return ok;
}

static int al_sfxr_readf(al_sfxr_Read8 const reader, void* const userdata, float* const fp) {
    uint32_t dword;

    int ok = al_sfxr_read32(reader, userdata, &dword);

    memcpy((void*)fp, (void*)&dword, sizeof(*fp));
    return ok;
}

static int al_sfxr_readi(al_sfxr_Read8 const reader, void* const userdata, int* const i) {
    uint32_t temp;
    int ok = al_sfxr_read32(reader, userdata, &temp);
    *i = (int)temp;
    return ok;
}

int al_sfxr_load(al_sfxr_Params* const params, al_sfxr_Read8 const reader, void* const userdata) {
    int version;
    int ok = al_sfxr_readi(reader, userdata, &version);

	if (version != 100 && version != 101 && version != 102) {
        return -1;
    }

    al_sfxr_zero(params);

    int wave_type;
    ok = ok || al_sfxr_readi(reader, userdata, &wave_type);
    params->wave_type = (al_sfxr_Wave)wave_type;

    if (version == 102) {
        ok = ok || al_sfxr_readf(reader, userdata, &params->sound_vol);
    }

    ok = ok || al_sfxr_readf(reader, userdata, &params->p_base_freq);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_freq_limit);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_freq_ramp);

    if (version >= 101) {
        ok = ok || al_sfxr_readf(reader, userdata, &params->p_freq_dramp);
    }

    ok = ok || al_sfxr_readf(reader, userdata, &params->p_duty);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_duty_ramp);

    ok = ok || al_sfxr_readf(reader, userdata, &params->p_vib_strength);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_vib_speed);
    float vib_delay; /* unused */
    ok = ok || al_sfxr_readf(reader, userdata, &vib_delay);

    ok = ok || al_sfxr_readf(reader, userdata, &params->p_env_attack);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_env_sustain);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_env_decay);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_env_punch);

    uint8_t filter_on; /* unused */
    ok = ok || reader(userdata, &filter_on);

    ok = ok || al_sfxr_readf(reader, userdata, &params->p_lpf_resonance);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_lpf_freq);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_lpf_ramp);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_hpf_freq);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_hpf_ramp);

    ok = ok || al_sfxr_readf(reader, userdata, &params->p_pha_offset);
    ok = ok || al_sfxr_readf(reader, userdata, &params->p_pha_ramp);

    ok = ok || al_sfxr_readf(reader, userdata, &params->p_repeat_speed);

    if (version >= 101) {
        ok = ok || al_sfxr_readf(reader, userdata, &params->p_arp_speed);
        ok = ok || al_sfxr_readf(reader, userdata, &params->p_arp_mod);
    }

    return ok;
}
#endif /* AL_SFXR_LOAD */

#if defined(AL_SFXR_GENERATE)
static void al_sfxr_clamp_value(float* const value, int const bipolar) {
    if (bipolar) {
        if (*value < -1.0f) {
            *value = -1.0f;
        }
    }
    else {
        if (*value < 0.0f) {
            *value = 0.0f;
        }
    }

    if (*value > 1.0f) {
        *value = 1.0f;
    }
}

static void al_sfxr_clamp(al_sfxr_Params* const params) {
    al_sfxr_clamp_value(&params->p_base_freq, 0);
    al_sfxr_clamp_value(&params->p_freq_ramp, 1);
    al_sfxr_clamp_value(&params->p_freq_dramp, 1);
    al_sfxr_clamp_value(&params->p_duty, 0);
    al_sfxr_clamp_value(&params->p_duty_ramp, 1);
    al_sfxr_clamp_value(&params->p_vib_strength, 0);
    al_sfxr_clamp_value(&params->p_vib_speed, 0);
    al_sfxr_clamp_value(&params->p_env_attack, 0);
    al_sfxr_clamp_value(&params->p_env_sustain, 0);
    al_sfxr_clamp_value(&params->p_env_decay, 0);
    al_sfxr_clamp_value(&params->p_env_punch, 0);
    al_sfxr_clamp_value(&params->p_lpf_resonance, 0);
    al_sfxr_clamp_value(&params->p_lpf_freq, 0);
    al_sfxr_clamp_value(&params->p_lpf_ramp, 1);
    al_sfxr_clamp_value(&params->p_hpf_freq, 0);
    al_sfxr_clamp_value(&params->p_hpf_ramp, 1);
    al_sfxr_clamp_value(&params->p_pha_offset, 1);
    al_sfxr_clamp_value(&params->p_pha_ramp, 1);
    al_sfxr_clamp_value(&params->p_repeat_speed, 0);
    al_sfxr_clamp_value(&params->p_arp_speed, 0);
    al_sfxr_clamp_value(&params->p_arp_mod, 1);
}

static void al_sfxr_mutate(float* const value, al_sfxr_Prng* const prng) {
    if (al_sfxr_randu(prng, 1)) {
        *value += al_sfxr_randf(prng, 0.1f) - 0.05f;
    }
}

void al_sfxr_generate(al_sfxr_Params* const params, al_sfxr_Preset const preset, unsigned const mutations, uint64_t const seed) {
    static const al_sfxr_Wave wave_types[] = {
        AL_SFXR_SQUARE,
        AL_SFXR_SAWTOOTH,
        AL_SFXR_SINEWAVE,
        AL_SFXR_NOISE
    };

    al_sfxr_Prng prng;
    al_sfxr_newprng(&prng, seed);

    al_sfxr_zero(params);

    switch (preset) {
        case AL_SFXR_RANDOM:
            params->wave_type = wave_types[al_sfxr_randu(&prng, 3)];
            params->p_base_freq = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 2.0f);

            if (al_sfxr_randu(&prng, 1)) {
                params->p_base_freq = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 3.0f) + 0.5f;
            }

            params->p_freq_limit = 0.0f;
            params->p_freq_ramp = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 5.0f);

            if (params->p_base_freq > 0.7f && params->p_freq_ramp > 0.2f) {
                params->p_freq_ramp = -params->p_freq_ramp;
            }

            if (params->p_base_freq < 0.2f && params->p_freq_ramp < -0.05f) {
                params->p_freq_ramp = -params->p_freq_ramp;
            }

            params->p_freq_dramp = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 3.0f);
            params->p_duty = al_sfxr_randf(&prng, 2.0f) - 1.0f;
            params->p_duty_ramp = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 3.0f);
            params->p_vib_strength = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 3.0f);
            params->p_vib_speed = al_sfxr_randf(&prng, 2.0f) - 1.0f;
            params->p_env_attack = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 3.0f);
            params->p_env_sustain = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 2.0f);
            params->p_env_decay = al_sfxr_randf(&prng, 2.0f) - 1.0f;
            params->p_env_punch = pow(al_sfxr_randf(&prng, 0.8f), 2.0f);

            if ((params->p_env_attack + params->p_env_sustain + params->p_env_decay) < 0.2f) {
                params->p_env_sustain += 0.2f + al_sfxr_randf(&prng, 0.3f);
                params->p_env_decay += 0.2f + al_sfxr_randf(&prng, 0.3f);
            }

            params->p_lpf_resonance = al_sfxr_randf(&prng, 2.0f) - 1.0f;
            params->p_lpf_freq = 1.0f - pow(al_sfxr_randf(&prng, 1.0f), 3.0f);
            params->p_lpf_ramp = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 3.0f);

            if (params->p_lpf_freq < 0.1f && params->p_lpf_ramp < -0.05f) {
                params->p_lpf_ramp = -params->p_lpf_ramp;
            }

            params->p_hpf_freq = pow(al_sfxr_randf(&prng, 1.0f), 5.0f);
            params->p_hpf_ramp = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 5.0f);
            params->p_pha_offset = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 3.0f);
            params->p_pha_ramp = pow(al_sfxr_randf(&prng, 2.0f) - 1.0f, 3.0f);
            params->p_repeat_speed = al_sfxr_randf(&prng, 2.0f) - 1.0f;
            params->p_arp_speed = al_sfxr_randf(&prng, 2.0f) - 1.0f;
            params->p_arp_mod = al_sfxr_randf(&prng, 2.0f) - 1.0f;

            break;

        case AL_SFXR_PICKUP:
            params->p_base_freq = 0.4f + al_sfxr_randf(&prng, 0.5f);
            params->p_env_attack = 0.0f;
            params->p_env_sustain = al_sfxr_randf(&prng, 0.1f);
            params->p_env_decay = 0.1f + al_sfxr_randf(&prng, 0.4f);
            params->p_env_punch = 0.3f + al_sfxr_randf(&prng, 0.3f);

            if (al_sfxr_randu(&prng, 1)) {
                params->p_arp_speed = 0.5f + al_sfxr_randf(&prng, 0.2f);
                params->p_arp_mod = 0.2f + al_sfxr_randf(&prng, 0.4f);
            }

            break;

        case AL_SFXR_LASER:
            params->wave_type = wave_types[al_sfxr_randu(&prng, 2)];

            if (params->wave_type == AL_SFXR_SINEWAVE && al_sfxr_randu(&prng, 1)) {
                params->wave_type = wave_types[al_sfxr_randu(&prng, 1)];
            }

            params->p_base_freq = 0.5f + al_sfxr_randf(&prng, 0.5f);
            params->p_freq_limit = params->p_base_freq - 0.2f - al_sfxr_randf(&prng, 0.6f);

            if (params->p_freq_limit < 0.2f) {
                params->p_freq_limit = 0.2f;
            }

            params->p_freq_ramp = -0.15f - al_sfxr_randf(&prng, 0.2f);

            if (al_sfxr_randu(&prng, 2) == 0) {
                params->p_base_freq = 0.3f + al_sfxr_randf(&prng, 0.6f);
                params->p_freq_limit = al_sfxr_randf(&prng, 0.1f);
                params->p_freq_ramp = -0.35f - al_sfxr_randf(&prng, 0.3f);
            }

            if (al_sfxr_randu(&prng, 1)) {
                params->p_duty = al_sfxr_randf(&prng, 0.5f);
                params->p_duty_ramp = al_sfxr_randf(&prng, 0.2f);
            }
            else {
                params->p_duty = 0.4f + al_sfxr_randf(&prng, 0.5f);
                params->p_duty_ramp = -al_sfxr_randf(&prng, 0.7f);
            }

            params->p_env_attack = 0.0f;
            params->p_env_sustain = 0.1f + al_sfxr_randf(&prng, 0.2f);
            params->p_env_decay = al_sfxr_randf(&prng, 0.4f);

            if (al_sfxr_randu(&prng, 1)) {
                params->p_env_punch = al_sfxr_randf(&prng, 0.3f);
            }

            if (al_sfxr_randu(&prng, 2) == 0) {
                params->p_pha_offset = al_sfxr_randf(&prng, 0.2f);
                params->p_pha_ramp = -al_sfxr_randf(&prng, 0.2f);
            }

            if (al_sfxr_randu(&prng, 1)) {
                params->p_hpf_freq = al_sfxr_randf(&prng, 0.3f);
            }

            break;

        case AL_SFXR_EXPLOSION:
            params->wave_type = AL_SFXR_NOISE;

            if (al_sfxr_randu(&prng, 1)) {
                params->p_base_freq = 0.1f + al_sfxr_randf(&prng, 0.4f);
                params->p_freq_ramp = -0.1f + al_sfxr_randf(&prng, 0.4f);
            }
            else {
                params->p_base_freq = 0.2f + al_sfxr_randf(&prng, 0.7f);
                params->p_freq_ramp = -0.2f - al_sfxr_randf(&prng, 0.2f);
            }

            params->p_base_freq *= params->p_base_freq;

            if (al_sfxr_randu(&prng, 4) == 0) {
                params->p_freq_ramp=0.0f;
            }

            if (al_sfxr_randu(&prng, 2) == 0) {
                params->p_repeat_speed = 0.3f + al_sfxr_randf(&prng, 0.5f);
            }

            params->p_env_attack = 0.0f;
            params->p_env_sustain = 0.1f + al_sfxr_randf(&prng, 0.3f);
            params->p_env_decay = al_sfxr_randf(&prng, 0.5f);

            if (al_sfxr_randu(&prng, 1) == 0) {
                params->p_pha_offset = -0.3f + al_sfxr_randf(&prng, 0.9f);
                params->p_pha_ramp = -al_sfxr_randf(&prng, 0.3f);
            }

            params->p_env_punch = 0.2f + al_sfxr_randf(&prng, 0.6f);

            if (al_sfxr_randu(&prng, 1)) {
                params->p_vib_strength = al_sfxr_randf(&prng, 0.7f);
                params->p_vib_speed = al_sfxr_randf(&prng, 0.6f);
            }

            if (al_sfxr_randu(&prng, 2) == 0) {
                params->p_arp_speed = 0.6f + al_sfxr_randf(&prng, 0.3f);
                params->p_arp_mod = 0.8f - al_sfxr_randf(&prng, 1.6f);
            }

            break;

        case AL_SFXR_POWERUP:
            if (al_sfxr_randu(&prng, 1)) {
                params->wave_type = AL_SFXR_SAWTOOTH;
            }
            else {
                params->p_duty = al_sfxr_randf(&prng, 0.6f);
            }

            if (al_sfxr_randu(&prng, 1)) {
                params->p_base_freq = 0.2f + al_sfxr_randf(&prng, 0.3f);
                params->p_freq_ramp = 0.1f + al_sfxr_randf(&prng, 0.4f);
                params->p_repeat_speed = 0.4f + al_sfxr_randf(&prng, 0.4f);
            }
            else {
                params->p_base_freq = 0.2f + al_sfxr_randf(&prng, 0.3f);
                params->p_freq_ramp = 0.05f + al_sfxr_randf(&prng, 0.2f);

                if (al_sfxr_randu(&prng, 1)) {
                    params->p_vib_strength = al_sfxr_randf(&prng, 0.7f);
                    params->p_vib_speed = al_sfxr_randf(&prng, 0.6f);
                }
            }

            params->p_env_attack = 0.0f;
            params->p_env_sustain = al_sfxr_randf(&prng, 0.4f);
            params->p_env_decay = 0.1f + al_sfxr_randf(&prng, 0.4f);
            break;

        case AL_SFXR_HIT:
            params->wave_type = wave_types[al_sfxr_randu(&prng, 2)];

            if (params->wave_type == AL_SFXR_SINEWAVE) {
                params->wave_type = AL_SFXR_NOISE;
            }

            if (params->wave_type == AL_SFXR_SQUARE) {
                params->p_duty = al_sfxr_randf(&prng, 0.6f);
            }

            params->p_base_freq = 0.2f + al_sfxr_randf(&prng, 0.6f);
            params->p_freq_ramp = -0.3f - al_sfxr_randf(&prng, 0.4f);
            params->p_env_attack = 0.0f;
            params->p_env_sustain = al_sfxr_randf(&prng, 0.1f);
            params->p_env_decay = 0.1f + al_sfxr_randf(&prng, 0.2f);

            if (al_sfxr_randu(&prng, 1)) {
                params->p_hpf_freq = al_sfxr_randf(&prng, 0.3f);
            }

            break;

        case AL_SFXR_JUMP:
            params->wave_type = AL_SFXR_SQUARE;
            params->p_duty = al_sfxr_randf(&prng, 0.6f);
            params->p_base_freq = 0.3f + al_sfxr_randf(&prng, 0.3f);
            params->p_freq_ramp = 0.1f + al_sfxr_randf(&prng, 0.2f);
            params->p_env_attack = 0.0f;
            params->p_env_sustain = 0.1f + al_sfxr_randf(&prng, 0.3f);
            params->p_env_decay = 0.1f + al_sfxr_randf(&prng, 0.2f);

            if (al_sfxr_randu(&prng, 1)) {
                params->p_hpf_freq = al_sfxr_randf(&prng, 0.3f);
            }

            if (al_sfxr_randu(&prng, 1)) {
                params->p_lpf_freq = 1.0f - al_sfxr_randf(&prng, 0.6f);
            }

            break;

        case AL_SFXR_BLIP:
            params->wave_type = wave_types[al_sfxr_randu(&prng, 1)];

            if (params->wave_type == AL_SFXR_SQUARE) {
                params->p_duty = al_sfxr_randf(&prng, 0.6f);
            }

            params->p_base_freq = 0.2f + al_sfxr_randf(&prng, 0.4f);
            params->p_env_attack = 0.0f;
            params->p_env_sustain = 0.1f + al_sfxr_randf(&prng, 0.1f);
            params->p_env_decay = al_sfxr_randf(&prng, 0.2f);
            params->p_hpf_freq = 0.1f;

            break;
    }

    al_sfxr_clamp(params);

    for (int i = 0; i < mutations; i++) {
        al_sfxr_mutate(&params->p_base_freq, &prng);
        al_sfxr_mutate(&params->p_freq_ramp, &prng);
        al_sfxr_mutate(&params->p_freq_dramp, &prng);
        al_sfxr_mutate(&params->p_duty, &prng);
        al_sfxr_mutate(&params->p_duty_ramp, &prng);
        al_sfxr_mutate(&params->p_vib_strength, &prng);
        al_sfxr_mutate(&params->p_vib_speed, &prng);
        al_sfxr_mutate(&params->p_env_attack, &prng);
        al_sfxr_mutate(&params->p_env_sustain, &prng);
        al_sfxr_mutate(&params->p_env_decay, &prng);
        al_sfxr_mutate(&params->p_env_punch, &prng);
        al_sfxr_mutate(&params->p_lpf_resonance, &prng);
        al_sfxr_mutate(&params->p_lpf_freq, &prng);
        al_sfxr_mutate(&params->p_lpf_ramp, &prng);
        al_sfxr_mutate(&params->p_hpf_freq, &prng);
        al_sfxr_mutate(&params->p_hpf_ramp, &prng);
        al_sfxr_mutate(&params->p_pha_offset, &prng);
        al_sfxr_mutate(&params->p_pha_ramp, &prng);
        al_sfxr_mutate(&params->p_repeat_speed, &prng);
        al_sfxr_mutate(&params->p_arp_speed, &prng);
        al_sfxr_mutate(&params->p_arp_mod, &prng);

        al_sfxr_clamp(params);
    }
}
#endif /* AL_SFXR_GENERATE */

#if defined(AL_SFXR_SAVE)
static int al_sfxr_write16(al_sfxr_Write8 const writer, void* const userdata, uint16_t const value) {
    int ok = writer(userdata, value & 0xff);
    return ok || writer(userdata, value >> 8);
}

static int al_sfxr_write32(al_sfxr_Write8 const writer, void* const userdata, uint32_t const value) {
    int ok = al_sfxr_write16(writer, userdata, value & 0xffff);
    return ok || al_sfxr_write16(writer, userdata, value >> 16);
}

static int al_sfxr_writef(al_sfxr_Write8 const writer, void* const userdata, float const value) {
    uint32_t dword;
    memcpy((void*)&dword, (void*)&value, sizeof(dword));

    return al_sfxr_write32(writer, userdata, dword);
}

static int al_sfxr_writei(al_sfxr_Write8 const writer, void* const userdata, int const value) {
    uint32_t temp = (uint32_t)value;
    return al_sfxr_write32(writer, userdata, temp);
}

int al_sfxr_save(al_sfxr_Params const* const params, al_sfxr_Write8 const writer, void* const userdata) {
    int ok = al_sfxr_writei(writer, userdata, 102); /* version */
    ok = ok || al_sfxr_writei(writer, userdata, (int)params->wave_type);

    ok = ok || al_sfxr_writef(writer, userdata, params->sound_vol);

    ok = ok || al_sfxr_writef(writer, userdata, params->p_base_freq);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_freq_limit);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_freq_ramp);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_freq_dramp);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_duty);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_duty_ramp);

    ok = ok || al_sfxr_writef(writer, userdata, params->p_vib_strength);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_vib_speed);
    ok = ok || al_sfxr_writef(writer, userdata, 0.0f); /* vib_delay, unused */

    ok = ok || al_sfxr_writef(writer, userdata, params->p_env_attack);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_env_sustain);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_env_decay);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_env_punch);

    ok = ok || writer(userdata, 0); /* filter_on, unused */

    ok = ok || al_sfxr_writef(writer, userdata, params->p_lpf_resonance);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_lpf_freq);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_lpf_ramp);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_hpf_freq);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_hpf_ramp);

    ok = ok || al_sfxr_writef(writer, userdata, params->p_pha_offset);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_pha_ramp);

    ok = ok || al_sfxr_writef(writer, userdata, params->p_repeat_speed);

    ok = ok || al_sfxr_writef(writer, userdata, params->p_arp_speed);
    ok = ok || al_sfxr_writef(writer, userdata, params->p_arp_mod);

    return ok;
}
#endif /* AL_SFXR_SAVE */

static void al_sfxr_resetsample(al_sfxr_Decoder* const decoder, int const restart) {
    if (!restart) {
        decoder->phase = 0;
    }

    decoder->fperiod = 100.0 / (decoder->params.p_base_freq * decoder->params.p_base_freq + 0.001);
    decoder->period = (int)decoder->fperiod;
    decoder->fmaxperiod = 100.0 / (decoder->params.p_freq_limit * decoder->params.p_freq_limit + 0.001);
    decoder->fslide = 1.0 - pow((double)decoder->params.p_freq_ramp, 3.0) * 0.01;
    decoder->fdslide = -pow((double)decoder->params.p_freq_dramp, 3.0) * 0.000001;
    decoder->square_duty = 0.5f - decoder->params.p_duty * 0.5f;
    decoder->square_slide = -decoder->params.p_duty_ramp * 0.00005f;

    if (decoder->params.p_arp_mod >= 0.0f) {
        decoder->arp_mod = 1.0 - pow((double)decoder->params.p_arp_mod, 2.0) * 0.9;
    }
    else {
        decoder->arp_mod = 1.0 + pow((double)decoder->params.p_arp_mod, 2.0) * 10.0;
    }

    decoder->arp_time = 0;
    decoder->arp_limit = (int)(pow(1.0f - decoder->params.p_arp_speed, 2.0f) * 20000 + 32);

    if (decoder->params.p_arp_speed == 1.0f) {
        decoder->arp_limit = 0;
    }

    if (!restart) {
        /* Reset filter */
        decoder->fltp = 0.0f;
        decoder->fltdp = 0.0f;
        decoder->fltw = pow(decoder->params.p_lpf_freq, 3.0f) * 0.1f;
        decoder->fltw_d = 1.0f + decoder->params.p_lpf_ramp * 0.0001f;
        decoder->fltdmp = 5.0f / (1.0f + pow(decoder->params.p_lpf_resonance, 2.0f) * 20.0f) * (0.01f + decoder->fltw);

        if (decoder->fltdmp > 0.8f) {
            decoder->fltdmp = 0.8f;
        }

        decoder->fltphp = 0.0f;
        decoder->flthp = pow(decoder->params.p_hpf_freq, 2.0f) * 0.1f;
        decoder->flthp_d = 1.0 + decoder->params.p_hpf_ramp * 0.0003f;

        /* Reset vibrato */
        decoder->vib_phase = 0.0f;
        decoder->vib_speed = pow(decoder->params.p_vib_speed, 2.0f) * 0.01f;
        decoder->vib_amp = decoder->params.p_vib_strength * 0.5f;

        /* Reset envelope */
        decoder->env_vol = 0.0f;
        decoder->env_stage = 0;
        decoder->env_time = 0;
        decoder->env_length[0] = (int)(decoder->params.p_env_attack * decoder->params.p_env_attack * 100000.0f);
        decoder->env_length[1] = (int)(decoder->params.p_env_sustain * decoder->params.p_env_sustain * 100000.0f);
        decoder->env_length[2] = (int)(decoder->params.p_env_decay * decoder->params.p_env_decay * 100000.0f);

        decoder->fphase = pow(decoder->params.p_pha_offset, 2.0f) * 1020.0f;

        if (decoder->params.p_pha_offset < 0.0f) {
            decoder->fphase = -decoder->fphase;
        }

        decoder->fdphase = pow(decoder->params.p_pha_ramp, 2.0f) * 1.0f;

        if (decoder->params.p_pha_ramp < 0.0f) {
            decoder->fdphase = -decoder->fdphase;
        }

        decoder->iphase = abs((int)decoder->fphase);
        decoder->ipp = 0;

        for (int i = 0; i < 1024; i++) {
            decoder->phaser_buffer[i] = 0.0f;
        }

        for(int i = 0; i < 32; i++) {
            decoder->noise_buffer[i] = al_sfxr_randf(&decoder->prng, 2.0f) - 1.0f;
        }

        decoder->rep_time = 0;
        decoder->rep_limit = (int)(pow(1.0f - decoder->params.p_repeat_speed, 2.0f) * 20000 + 32);

        if (decoder->params.p_repeat_speed == 0.0f) {
            decoder->rep_limit = 0;
        }
    }
}

void al_sfxr_start(al_sfxr_Decoder* const decoder, al_sfxr_Params const* const params, uint64_t const seed) {
    decoder->params = *params;
    al_sfxr_newprng(&decoder->prng, seed);
    al_sfxr_resetsample(decoder, 0);

    decoder->playing_sample = 1;
}

void al_sfxr_start_quick(al_sfxr_Decoder* const decoder, al_sfxr_Params const* const params) {
    al_sfxr_start(decoder, params, UINT64_C(0x89866ae81aa30a2b));
}

void al_sfxr_restart(al_sfxr_Decoder* const decoder) {
    al_sfxr_resetsample(decoder, 0);
}

#if defined(AL_SFXR_INT16_MONO) || defined(AL_SFXR_INT16_STEREO) || \
    defined(AL_SFXR_FLOAT_MONO) || defined(AL_SFXR_FLOAT_STEREO)
static float al_sfxr_produce(al_sfxr_Decoder* const decoder) {
    if (!decoder->playing_sample) {
        return 0.0f;
    }

    decoder->rep_time++;

    if (decoder->rep_limit != 0 && decoder->rep_time >= decoder->rep_limit) {
        decoder->rep_time = 0;
        al_sfxr_resetsample(decoder, 1);
    }

    /* Frequency envelopes/arpeggios */
    decoder->arp_time++;

    if (decoder->arp_limit != 0 && decoder->arp_time >= decoder->arp_limit) {
        decoder->arp_limit = 0;
        decoder->fperiod *= decoder->arp_mod;
    }

    decoder->fslide += decoder->fdslide;
    decoder->fperiod *= decoder->fslide;

    if (decoder->fperiod > decoder->fmaxperiod) {
        decoder->fperiod = decoder->fmaxperiod;

        if (decoder->params.p_freq_limit > 0.0f) {
            decoder->playing_sample = 0;
            return 0.0f;
        }
    }

    float rfperiod = decoder->fperiod;

    if (decoder->vib_amp > 0.0f) {
        decoder->vib_phase += decoder->vib_speed;
        rfperiod = decoder->fperiod * (1.0 + sin(decoder->vib_phase) * decoder->vib_amp);
    }

    decoder->period = (int)rfperiod;

    if (decoder->period < 8) {
        decoder->period = 8;
    }

    decoder->square_duty += decoder->square_slide;

    if (decoder->square_duty < 0.0f) {
        decoder->square_duty = 0.0f;
    }

    if (decoder->square_duty > 0.5f) {
        decoder->square_duty = 0.5f;
    }

    /* Volume envelope */
    decoder->env_time++;

    if (decoder->env_time > decoder->env_length[decoder->env_stage]) {
        decoder->env_time = 0;
        decoder->env_stage++;

        if (decoder->env_stage == 3) {
            decoder->playing_sample = 0;
            return 0.0f;
        }
    }

    if (decoder->env_stage == 0) {
        decoder->env_vol = (float)decoder->env_time / decoder->env_length[0];
    }
    else if (decoder->env_stage == 1) {
        decoder->env_vol = 1.0f + pow(1.0f - (float)decoder->env_time / decoder->env_length[1], 1.0f) *
                           2.0f * decoder->params.p_env_punch;
    }
    else if (decoder->env_stage == 2) {
        decoder->env_vol = 1.0f - (float)decoder->env_time / decoder->env_length[2];
    }

    /* Phaser step */
    decoder->fphase += decoder->fdphase;
    decoder->iphase = abs((int)decoder->fphase);

    if (decoder->iphase > 1023) {
        decoder->iphase = 1023;
    }

    if (decoder->flthp_d != 0.0f) {
        decoder->flthp *= decoder->flthp_d;

        if (decoder->flthp < 0.00001f) {
            decoder->flthp = 0.00001f;
        }

        if (decoder->flthp > 0.1f) {
            decoder->flthp = 0.1f;
        }
    }

    float ssample = 0.0f;

    /* 8x supersampling */
    for (int si = 0; si < 8; si++) {
        float sample = 0.0f;

        decoder->phase++;

        if (decoder->phase >= decoder->period) {
            decoder->phase %= decoder->period;

            if (decoder->params.wave_type == AL_SFXR_NOISE) {
                for (int i = 0; i < 32; i++) {
                    decoder->noise_buffer[i] = al_sfxr_randf(&decoder->prng, 2.0f) - 1.0f;
                }
            }
        }

        /* Base waveform */
        float fp = (float)decoder->phase / decoder->period;

        switch (decoder->params.wave_type) {
            case AL_SFXR_SQUARE:
                if (fp < decoder->square_duty) {
                    sample = 0.5f;
                }
                else {
                    sample = -0.5f;
                }

                break;

            case AL_SFXR_SAWTOOTH:
                sample = 1.0f - fp * 2.0f;
                break;

            case AL_SFXR_SINEWAVE:
                sample = (float)sin(fp * 2.0f * 3.14159265358979323846f);
                break;

            case AL_SFXR_NOISE:
                sample = decoder->noise_buffer[decoder->phase * 32 / decoder->period];
                break;
        }

        /* Low-pass filter */
        float pp = decoder->fltp;
        decoder->fltw *= decoder->fltw_d;

        if (decoder->fltw < 0.0f) {
            decoder->fltw = 0.0f;
        }

        if (decoder->fltw > 0.1f) {
            decoder->fltw = 0.1f;
        }

        if (decoder->params.p_lpf_freq != 1.0f) {
            decoder->fltdp += (sample - decoder->fltp) * decoder->fltw;
            decoder->fltdp -= decoder->fltdp * decoder->fltdmp;
        }
        else {
            decoder->fltp = sample;
            decoder->fltdp = 0.0f;
        }

        decoder->fltp += decoder->fltdp;

        /* High-pass filter */
        decoder->fltphp += decoder->fltp - pp;
        decoder->fltphp -= decoder->fltphp * decoder->flthp;
        sample = decoder->fltphp;

        /* Phaser */
        decoder->phaser_buffer[decoder->ipp & 1023] = sample;
        sample += decoder->phaser_buffer[(decoder->ipp - decoder->iphase + 1024) & 1023];
        decoder->ipp = (decoder->ipp + 1) & 1023;

        /* Final accumulation and envelope application */
        ssample += sample * decoder->env_vol;
    }

    ssample = ssample / 8;
    ssample *= 2.0f * decoder->params.sound_vol;

    if (ssample > 1.0f) {
        ssample = 1.0f;
    }
    else if (ssample < -1.0f) {
        ssample = -1.0f;
    }

    return ssample;
}

#if defined(AL_SFXR_INT16_MONO)
size_t al_sfxr_produce1i(al_sfxr_Decoder* const decoder, int16_t* frames, size_t const num_frames) {
    size_t i = 0;

    for (; i < num_frames; i++, frames++) {
        float const samplef = al_sfxr_produce(decoder);

        if (!decoder->playing_sample) {
            break;
        }

        int16_t const sample = (int16_t)(samplef * 32767.0f);
        *frames = sample;
    }

    return i;
}
#endif /* AL_SFXR_INT16_MONO */

#if defined(AL_SFXR_INT16_STEREO)
size_t al_sfxr_produce2i(al_sfxr_Decoder* const decoder, int16_t* frames, size_t const num_frames) {
    size_t i = 0;

    for (; i < num_frames; i++, frames += 2) {
        float const samplef = al_sfxr_produce(decoder);

        if (!decoder->playing_sample) {
            break;
        }

        int16_t const sample = (int16_t)(samplef * 32767.0f);
        frames[0] = sample;
        frames[1] = sample;
    }

    return i;
}
#endif /* AL_SFXR_INT16_STEREO */

#if defined(AL_SFXR_FLOAT_MONO)
size_t al_sfxr_produce1f(al_sfxr_Decoder* const decoder, float* frames, size_t const num_frames) {
    size_t i = 0;

    for (; i < num_frames; i++, frames++) {
        *frames = al_sfxr_produce(decoder);

        if (!decoder->playing_sample) {
            break;
        }
    }

    return i;
}
#endif /* AL_SFXR_FLOAT_MONO */

#if defined(AL_SFXR_FLOAT_STEREO)
size_t al_sfxr_produce2f(al_sfxr_Decoder* const decoder, float* frames, size_t const num_frames) {
    size_t i = 0;

    for (; i < num_frames; i++, frames += 2) {
        *frames = al_sfxr_produce(decoder);

        if (!decoder->playing_sample) {
            break;
        }

        frames[1] = *frames;
    }

    return i;
}
#endif /* AL_SFXR_FLOAT_STEREO */

#endif /* AL_SFXR_INT16_MONO || AL_SFXR_INT16_STEREO ||
          AL_SFXR_FLOAT_MONO || AL_SFXR_FLOAT_STEREO */

#endif /* AL_SFXR_IMPLEMENTATION */
