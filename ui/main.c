/*
The MIT License (MIT)

Copyright (c) 2007 Tomas Pettersson (original implementation)
Copyright (c) 2020 Andre Leiradella (changes to use al_sfxr)

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

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <tinyfiledialogs.h>

#include "icon.h"
#include "ld48.h"
#include "font.h"

/*---------------------------------------------------------------------------*/
/* al_sfxr config and inclusion */
#define AL_SFXR_IMPLEMENTATION
#define AL_SFXR_GENERATE
#define AL_SFXR_LOAD
#define AL_SFXR_SAVE
#define AL_SFXR_INT16_MONO
#include <al_sfxr.h>
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* dr_wav config and inclusion */
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
/*---------------------------------------------------------------------------*/

typedef struct sfxr_params_t sfxr_params_t;

struct sfxr_params_t {
    int generate;
    sfxr_params_t const* previous;

    /* if generate is true */
    al_sfxr_Preset preset;
    unsigned mutations;
    uint64_t seed;

    /* if generate is false */
    al_sfxr_Params params;
};

static SDL_Window* s_window = NULL;
static SDL_Renderer* s_renderer = NULL;
static SDL_Texture* s_framebuffer = NULL;
static SDL_Surface* s_screen = NULL;

static int s_mouse_x = 0, s_mouse_y = 0, s_mouse_px = 0, s_mouse_py = 0;
static int s_mouse_left = 0, s_mouse_right = 0, s_mouse_middle = 0;
static int s_mouse_left_click = 0, s_mouse_right_click = 0, s_mouse_middle_click = 0;

static sfxr_params_t s_curparams, s_prevparams;
static sfxr_params_t const* s_history = NULL;
static al_sfxr_Decoder s_decoder = {0};

static struct {char const* name; al_sfxr_Preset preset;} const s_categories[8] = {
    {"Pickup/Coin", AL_SFXR_PICKUP},
    {"Laser/Shoot", AL_SFXR_LASER},
    {"Explosion", AL_SFXR_EXPLOSION},
    {"Power Up", AL_SFXR_POWERUP},
    {"Hit/Hurt", AL_SFXR_HIT},
    {"Jump", AL_SFXR_JUMP},
    {"Blip/Select", AL_SFXR_BLIP},
    {"Random", AL_SFXR_RANDOM}
};

static SDL_Surface* s_ld48 = NULL;
static SDL_Surface* s_font = NULL;

static int s_first_frame = 1;
static int s_refresh_counter = 0;
static float *s_vselected = NULL;
static int s_vcurbutton = -1;
static int s_playing_sample = 0;
static int s_draw_count = 0;
static uint64_t s_seed = 1;

static void push(sfxr_params_t const* const params) {
    sfxr_params_t* top = (sfxr_params_t*)malloc(sizeof(*top));
    *top = *params;
    top->previous = s_history;
    s_history = top;
}

static void pop(sfxr_params_t* const params) {
    sfxr_params_t const* const top = s_history;
    *params = *top;
    s_history = top->previous;
    free((void*)top);
}

static void play_sample(void) {
    int equal = 0;

    if (s_curparams.generate == 1) {
        equal = s_curparams.preset == s_prevparams.preset &&
                s_curparams.mutations == s_prevparams.mutations &&
                s_curparams.seed == s_prevparams.seed;
    }
    else {
        equal = memcmp(&s_curparams.params, &s_prevparams.params, sizeof(s_curparams.params)) == 0;
    }

    if (!equal) {
        push(&s_prevparams);
        s_prevparams = s_curparams;
    }

    al_sfxr_start_quick(&s_decoder, &s_curparams.params);
    s_playing_sample = 1;
}

static int fpreader(void* const userdata, uint8_t* const byte) {
    FILE* const fp = (FILE*)userdata;
    return fread(byte, 1, 1, fp) != 1;
}

static void load_sound(void) {
    static char const* const extensions[1] = {
        "*.sfxr"
    };

    char const* filename = tinyfd_openFileDialog(
	    "Load SFXR Sound",
	    NULL,
	    1,
	    extensions,
	    "SFXR Sound files",
	    0
    );

    if (filename == NULL) {
        return;
    }

    FILE* const fp = fopen(filename, "rb");

    if (fp == NULL) {
        fprintf(stderr, "Error opening \"%s\": %s\n", filename, strerror(errno));
        return;
    }

    int const res = al_sfxr_load(&s_curparams.params, fpreader, fp);
    fclose(fp);

    if (res != 0) {
        fprintf(stderr, "Error loading \"%s\"\n", filename);
        return;
    }

    s_curparams.generate = 0;
    play_sample();
}

static int fpwriter(void* const userdata, uint8_t const byte) {
    FILE* const fp = (FILE*)userdata;
    return fwrite(&byte, 1, 1, fp) != 1;
}

static void save_sound(void) {
    static char const* const extensions[1] = {
        "*.sfxr"
    };

    char const* filename = tinyfd_saveFileDialog(
        "Save SFXR Sound",
        NULL,
        1,
        extensions,
        "SFXR Sound files"
    );

    if (filename == NULL) {
        return;
    }

    FILE* const fp = fopen(filename, "wb");

    if (fp == NULL) {
        fprintf(stderr, "Error opening \"%s\": %s\n", filename, strerror(errno));
        return;
    }

    int const res = al_sfxr_save(&s_curparams.params, fpwriter, fp);
    fclose(fp);

    if (res != 0) {
        fprintf(stderr, "Error saving \"%s\"\n", filename);
    }
}

static void export_wav(void) {
    static char const* const extensions[1] = {
        "*.wav"
    };

    char const* filename = tinyfd_saveFileDialog(
        "Save WAV",
        NULL,
        1,
        extensions,
        "WAV files"
    );

    if (filename == NULL) {
        return;
    }

    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_PCM;
    format.channels = 1;
    format.sampleRate = 44100;
    format.bitsPerSample = 16;

    drwav wav;
    
    if (!drwav_init_file_write(&wav, filename, &format, NULL)) {
        fprintf(stderr, "Error exporting WAV \"%s\": %s\n", filename, strerror(errno));
        return;
    }

    al_sfxr_Decoder decoder;
    al_sfxr_start_quick(&decoder, &s_curparams.params);

    while (1) {
        int16_t samples[256];
        size_t const samples_written = al_sfxr_produce1i(&decoder, samples, sizeof(samples) / sizeof(samples[0]));

        if (samples_written == 0) {
            break;
        }

        drwav_write_pcm_frames(&wav, samples_written, samples);
    }

    drwav_uninit(&wav);
}

static SDL_Surface* load_image(int const width, int const height, uint32_t const* const abgr) {
    Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = UINT32_C(0xff000000);
    gmask = UINT32_C(0x00ff0000);
    bmask = UINT32_C(0x0000ff00);
    amask = UINT32_C(0x000000ff);
#else
    rmask = UINT32_C(0x000000ff);
    gmask = UINT32_C(0x0000ff00);
    bmask = UINT32_C(0x00ff0000);
    amask = UINT32_C(0xff000000);
#endif

    SDL_Surface* const image = SDL_CreateRGBSurfaceFrom((void*)abgr, width, height, 32, 4 * width, rmask, gmask, bmask, amask);

    if (image == NULL) {
        fprintf(stderr, "SDL_CreateRGBSurfaceFrom: %s\n", SDL_GetError());
        return NULL;
    }

    return image;
}

static Uint32 map_color(uint32_t const rgb) {
    return SDL_MapRGB(s_screen->format, (rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255);
}

static int mouse_in_box(int const x, int const y, int const w, int const h) {
    return s_mouse_x >= x && s_mouse_x < (x + w) && s_mouse_y >= y && s_mouse_y < (y + h);
}

static void draw_text(int const sx, int const sy, uint32_t const color, char const* const fmt, ...) {
    (void)color;

    char string[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(string, sizeof(string), fmt, args);
    va_end(args);

    size_t const len = strlen(string);

    SDL_Rect src;
    src.w = 8;
    src.h = 8;

    SDL_Rect dst;
    dst.x = sx;
    dst.y = sy;

    for (size_t i = 0; i < len; i++) {
        int const k = (string[i] >= 0x60 ? string[i] ^ 0xe0 : string[i]) - 32;
        int const x = k % 32;
        int const y = k / 32;

        src.x = x * 8;
        src.y = y * 8;
        
        if (SDL_BlitSurface(s_font, &src, s_screen, &dst) != 0) {
            fprintf(stderr, "SDL_BlitSurface: %s\n", SDL_GetError());
            return;
        }

        dst.x += 8;
    }
}

static void draw_bar(int const sx, int const sy, int const w, int const h, uint32_t const color) {
    SDL_Rect dst;
    dst.x = sx;
    dst.y = sy;
    dst.w = w;
    dst.h = h;

    if (SDL_FillRect(s_screen, &dst, map_color(color)) != 0) {
        fprintf(stderr, "SDL_FillRect: %s\n", SDL_GetError());
    }
}

static int button(int const x, int const y, int const highlight, char const* const text, int const id) {
    Uint32 color1 = 0x000000;
    Uint32 color2 = 0xa09088;
    Uint32 color3 = 0x000000;

    int const hover = mouse_in_box(x, y, 100, 17);

    if (hover && s_mouse_left_click) {
        s_vcurbutton = id;
    }

    int const current = s_vcurbutton == id;

    if (highlight) {
        color1 = 0x000000;
        color2 = 0x988070;
        color3 = 0xfff0e0;
    }

    if (current && hover) {
        color1 = 0xa09088;
        color2 = 0xfff0e0;
        color3 = 0xa09088;
    }

    draw_bar(x - 1, y-1, 102, 19, color1);
    draw_bar(x, y, 100, 17, color2);
    draw_text(x + 5, y + 5, color3, text);

    return current && hover && !s_mouse_left;
}

static void slider(int const x, int const y, float* const value, int const bipolar, char const* const text) {
    float const old_value = *value;

    if (mouse_in_box(x, y, 100, 10)) {
        if (s_mouse_left_click) {
            s_vselected = value;
        }

        if (s_mouse_right_click) {
            *value = 0.0f;
        }
    }

    float mv = (float)(s_mouse_x - s_mouse_px);

    if (s_vselected != value) {
        mv = 0.0f;
    }

    if (bipolar) {
        *value += mv * 0.005f;

        if (*value < -1.0f) {
            *value = -1.0f;
        }

        if (*value > 1.0f) {
            *value = 1.0f;
        }
    }
    else {
        *value += mv * 0.0025f;

        if (*value < 0.0f) *value = 0.0f;
        if (*value > 1.0f) *value = 1.0f;
    }

    if (*value != old_value) {
        s_curparams.generate = s_first_frame;
    }

    draw_bar(x - 1, y, 102, 10, 0x000000);

    int ival = (int)(*value * 99);

    if (bipolar) {
        ival = (int)(*value * 49.5f + 49.5f);
    }

    draw_bar(x, y + 1, ival, 8, 0xf0c090);
    draw_bar(x + ival, y + 1, 100 - ival, 8, 0x807060);
    draw_bar(x + ival, y + 1, 1, 8, 0xffffff);

    if (bipolar) {
        draw_bar(x + 50, y - 1, 1, 3, 0x000000);
        draw_bar(x + 50, y + 8, 1, 3, 0x000000);
    }

    int const disabled = s_curparams.params.wave_type != AL_SFXR_SQUARE &&
                         (value == &s_curparams.params.p_duty || value == &s_curparams.params.p_duty_ramp);
    uint32_t const tcol = disabled ? 0x808080 : 0x000000;

    draw_text(x - 4 - strlen(text) * 8, y + 1, tcol, text);
}

void draw_screen(void)
{
    int redraw = 1;

    if (!s_first_frame && (s_mouse_x - s_mouse_px) == 0 && (s_mouse_y - s_mouse_py) == 0 && !s_mouse_left && !s_mouse_right) {
        redraw = 0;
    }

    if (!s_mouse_left) {
        if (s_vselected != NULL || s_vcurbutton > -1) {
            redraw = 1;
            s_refresh_counter = 2;
        }

        s_vselected = NULL;
    }

    if (s_refresh_counter > 0) {
        s_refresh_counter--;
        redraw = 1;
    }

    if (s_playing_sample) {
        redraw = 1;
    }

    if (s_draw_count++ > 20) {
        redraw = 1;
        s_draw_count = 0;
    }

    if (!redraw) {
        return;
    }

    int do_play = 0;

    if (SDL_FillRect(s_screen, NULL, map_color(0xc0b090)) != 0) {
        fprintf(stderr, "SDL_FillRect: %s\n", SDL_GetError());
    }

    draw_text(10, 10, 0x504030, "Generator");

    static size_t const max_categories = sizeof(s_categories) / sizeof(s_categories[0]);

    for (int i = 0; i < max_categories; i++) {
        if (button(5, 30 + i * 30, 0, s_categories[i].name, 300 + i)) {
            s_curparams.generate = 1;
            s_curparams.preset = s_categories[i].preset;
            s_curparams.mutations = 0;
            s_curparams.seed = s_seed++;

            al_sfxr_generate(&s_curparams.params, s_curparams.preset, s_curparams.mutations, s_curparams.seed);
            do_play = 1;
        }
    }

    if (button(5, 30 + max_categories * 30, !s_curparams.generate, "Mutate", 30) && s_curparams.generate) {
        s_curparams.generate = 1;
        s_curparams.mutations++;

        al_sfxr_generate(&s_curparams.params, s_curparams.preset, s_curparams.mutations, s_curparams.seed);
        do_play = 1;
    }

    if (button(5, 60 + max_categories * 30, s_history == NULL, "Back", 300 + max_categories) && s_history != NULL) {
        pop(&s_curparams);
        s_prevparams = s_curparams;
        do_play = 1;
    }

    if (button(5, 120 + max_categories * 30, !s_curparams.generate, "Copy", 300 + max_categories) && s_curparams.generate) {
        char const* preset = NULL;

        switch (s_curparams.preset) {
            case AL_SFXR_RANDOM:    preset = "AL_SFXR_RANDOM"; break;
            case AL_SFXR_PICKUP:    preset = "AL_SFXR_PICKUP"; break;
            case AL_SFXR_LASER:     preset = "AL_SFXR_LASER"; break;
            case AL_SFXR_EXPLOSION: preset = "AL_SFXR_EXPLOSION"; break;
            case AL_SFXR_POWERUP:   preset = "AL_SFXR_POWERUP"; break;
            case AL_SFXR_HIT:       preset = "AL_SFXR_HIT"; break;
            case AL_SFXR_JUMP:      preset = "AL_SFXR_JUMP"; break;
            case AL_SFXR_BLIP:      preset = "AL_SFXR_BLIP"; break;
        }

        char cmd[256];

        snprintf(
            cmd, sizeof(cmd),
            "al_sfxr_generate(&params, %s, %u, %" PRIu64 ");",
            preset,
            s_curparams.mutations,
            s_curparams.seed
        );

        if (SDL_SetClipboardText(cmd) != 0) {
            fprintf(stderr, "SDL_SetClipboardText: %s\n", SDL_GetError());
        }
    }

    draw_bar(110, 0, 2, 480, 0x000000);
    draw_text(120, 10, 0x504030, "Manual Settings");

    SDL_Rect dst;
    dst.x = 8;
    dst.y = 440;

    if (SDL_BlitSurface(s_ld48, NULL, s_screen, &dst) != 0) {
        fprintf(stderr, "SDL_BlitSurface: %s\n", SDL_GetError());
    }

    if (button(130, 30, s_curparams.params.wave_type == AL_SFXR_SQUARE, "Square Wave", 10)) {
        s_curparams.generate = 0;
        s_curparams.params.wave_type = AL_SFXR_SQUARE;
    }

    if (button(250, 30, s_curparams.params.wave_type == AL_SFXR_SAWTOOTH, "Sawtooth", 11)) {
        s_curparams.generate = 0;
        s_curparams.params.wave_type = AL_SFXR_SAWTOOTH;
    }

    if (button(370, 30, s_curparams.params.wave_type == AL_SFXR_SINEWAVE, "Sine Wave", 12)) {
        s_curparams.generate = 0;
        s_curparams.params.wave_type = AL_SFXR_SINEWAVE;
    }

    if (button(490, 30, s_curparams.params.wave_type == AL_SFXR_NOISE, "Noise", 13)) {
        s_curparams.generate = 0;
        s_curparams.params.wave_type = AL_SFXR_NOISE;
    }

    draw_text(515, 170, 0x000000, "Volume");
    draw_bar(490 - 1 - 1 + 60, 180 - 1, 42 + 2, 10 + 2, 0xFF0000);
	draw_bar(490 - 1 - 1 + 60, 180 - 1 + 5, 70, 2, 0x000000);
	draw_bar(490 - 1 - 1 + 60 + 68, 180 - 1 + 5, 2, 205, 0x000000);
	draw_bar(490 - 1 - 1 + 60, 380 - 1 + 9, 70, 2, 0x000000);
    slider(490, 180, &s_curparams.params.sound_vol, 0, " ");

    if (button(490, 200, 0, "Play Sound", 20)) {
        do_play = 1;
    }

    if (button(490, 230, 0, "Load Sound", 15)) {
        load_sound();
    }

    if (button(490, 260, 0, "Save Sound", 15)) {
        save_sound();
    }

    if (button(490, 380, 0, "Export .WAV", 16)) {
        export_wav();
    }

    int ypos = 4, xpos = 350;

    draw_bar(xpos - 190, ypos * 18 - 5, 300, 2, 0x0000000);

    slider(xpos, (ypos++) * 18, &s_curparams.params.p_env_attack, 0, "Attack Time");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_env_sustain, 0, "Sustain Time");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_env_punch, 0, "Sustain Punch");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_env_decay, 0, "Decay Time");

    draw_bar(xpos - 190, ypos * 18 - 5, 300, 2, 0x0000000);

    slider(xpos, (ypos++) * 18, &s_curparams.params.p_base_freq, 0, "Start Frequency");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_freq_limit, 0, "Min Frequency");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_freq_ramp, 1, "Slide");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_freq_dramp, 1, "Delta Slide");

    slider(xpos, (ypos++) * 18, &s_curparams.params.p_vib_strength, 0, "Vibrato Depth");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_vib_speed, 0, "Vibrato Speed");

    draw_bar(xpos - 190, ypos * 18 - 5, 300, 2, 0x0000000);

    slider(xpos, (ypos++) * 18, &s_curparams.params.p_arp_mod, 1, "Change Amount");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_arp_speed, 0, "Change Speed");

    draw_bar(xpos - 190, ypos * 18 - 5, 300, 2, 0x0000000);

    slider(xpos, (ypos++) * 18, &s_curparams.params.p_duty, 0, "Square Duty");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_duty_ramp, 1, "Duty Sweep");

    draw_bar(xpos - 190, ypos * 18 - 5, 300, 2, 0x0000000);

    slider(xpos, (ypos++) * 18, &s_curparams.params.p_repeat_speed, 0, "Repeat Speed");

    draw_bar(xpos - 190, ypos * 18 - 5, 300, 2, 0x0000000);

    slider(xpos, (ypos++) * 18, &s_curparams.params.p_pha_offset, 1, "Phaser Offset");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_pha_ramp, 1, "Phaser Sweep");

    draw_bar(xpos - 190, ypos * 18 - 5, 300, 2, 0x0000000);

    slider(xpos, (ypos++) * 18, &s_curparams.params.p_lpf_freq, 0, "LP Filter Cutoff");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_lpf_ramp, 1, "LP Filter Cutoff Sweep");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_lpf_resonance, 0, "LP Filter Resonance");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_hpf_freq, 0, "HP Filter Cutoff");
    slider(xpos, (ypos++) * 18, &s_curparams.params.p_hpf_ramp, 1, "HP Filter Cutoff Sweep");

    draw_bar(xpos - 190, ypos * 18 - 5, 300, 2, 0x0000000);

    draw_bar(xpos - 190, 4 * 18 - 5, 1, (ypos - 4) * 18, 0x0000000);
    draw_bar(xpos - 190 + 299, 4 * 18 - 5, 1, (ypos - 4) * 18, 0x0000000);

    if (do_play) {
        play_sample();
    }

    if (!s_mouse_left) {
        s_vcurbutton = -1;
    }

    s_first_frame = 0;
}

static void audio_callback(void* const userdata, Uint8* const stream, int const len) {
    (void)userdata;

    if (s_playing_sample) {
        size_t const frames_written = al_sfxr_produce1i(&s_decoder, (int16_t*)stream, len / sizeof(int16_t));
        size_t const bytes_written = frames_written * sizeof(int16_t);

        if (bytes_written < (size_t)len) {
            memset(stream + bytes_written, 0, (size_t)len - bytes_written);
        }
    }
}

static int init_sdl(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return -1;
    }

    atexit(SDL_Quit);

    s_window = SDL_CreateWindow("al_sfxr", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);

    if (s_window == NULL) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Surface* icon = load_image(icon_width, icon_height, icon_abgr);

    if (icon != NULL) {
        SDL_SetWindowIcon(s_window, icon);
        SDL_FreeSurface(icon);
    }

    s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (s_renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
error1:
        SDL_DestroyWindow(s_window);
        return -1;
    }

    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear")) {
        fprintf(stderr, "SDL_SetHint: %s\n", SDL_GetError());
error2:
        SDL_DestroyRenderer(s_renderer);
        goto error1;
    }

    s_framebuffer = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 640, 480);

    if (s_framebuffer == NULL) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        goto error2;
    }

    Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = UINT32_C(0xff000000);
    gmask = UINT32_C(0x00ff0000);
    bmask = UINT32_C(0x0000ff00);
    amask = UINT32_C(0x000000ff);
#else
    rmask = UINT32_C(0x000000ff);
    gmask = UINT32_C(0x0000ff00);
    bmask = UINT32_C(0x00ff0000);
    amask = UINT32_C(0xff000000);
#endif

    s_screen = SDL_CreateRGBSurface(0, 640, 480, 32, rmask, gmask, bmask, amask);

    SDL_AudioSpec des;
    des.freq = 44100;
    des.format = AUDIO_S16SYS;
    des.channels = 1;
    des.samples = 512;
    des.callback = audio_callback;
    des.userdata = NULL;

    if (SDL_OpenAudio(&des, NULL) != 0) {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
error3:
        SDL_DestroyTexture(s_framebuffer);
        goto error2;
    }

    SDL_PauseAudio(0);

    s_ld48 = load_image(ld48_width, ld48_height, ld48_abgr);

    if (s_ld48 == NULL) {
error4:
        SDL_CloseAudio();
        goto error3;
    }

    s_font = load_image(font_width, font_height, font_abgr);

    if (s_font == NULL) {
        SDL_FreeSurface(s_ld48);
        goto error4;
    }

    return 0;
}

static void run_loop(void) {
    SDL_Event e;

    while (1) {
        if (SDL_PollEvent(&e) && e.type == SDL_QUIT) {
            return;
        }

        /* Update the mouse */
        s_mouse_px = s_mouse_x;
        s_mouse_py = s_mouse_y;

        Uint8 const buttons = SDL_GetMouseState(&s_mouse_x, &s_mouse_y);

        int const mouse_left_p = s_mouse_left;
        int const mouse_right_p = s_mouse_right;
        int const mouse_middle_p = s_mouse_middle;

        s_mouse_left = buttons & SDL_BUTTON(1);
        s_mouse_right = buttons & SDL_BUTTON(3);
        s_mouse_middle = buttons & SDL_BUTTON(2);

        s_mouse_left_click = s_mouse_left && !mouse_left_p;
        s_mouse_right_click = s_mouse_right && !mouse_right_p;
        s_mouse_middle_click = s_mouse_middle && !mouse_middle_p;

        draw_screen();
        SDL_UpdateTexture(s_framebuffer, NULL, s_screen->pixels, 640 * sizeof(Uint32));

        SDL_RenderClear(s_renderer);
        SDL_RenderCopy(s_renderer, s_framebuffer, NULL, NULL);
        SDL_RenderPresent(s_renderer);
    }
}

static void uninit_sdl(void) {
    SDL_FreeSurface(s_font);
    SDL_FreeSurface(s_ld48);
}

int main(int const argc, char const* argv[]) {
    s_curparams.generate = 1;
    s_curparams.preset = AL_SFXR_POWERUP;
    s_curparams.mutations = 0;
    s_curparams.seed = s_seed++;

    al_sfxr_generate(&s_curparams.params, s_curparams.preset, s_curparams.mutations, s_curparams.seed);
    s_prevparams = s_curparams;

    if (init_sdl() != 0) {
        return EXIT_FAILURE;
    }

    play_sample();

    run_loop();
    uninit_sdl();
    return EXIT_SUCCESS;
}
