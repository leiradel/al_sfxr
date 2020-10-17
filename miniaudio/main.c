#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*---------------------------------------------------------------------------*/
/* al_sfxr config and inclusion */
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* al_sfxr config and inclusion */
#define AL_SFXR_IMPLEMENTATION
#define AL_SFXR_GENERATE
#define AL_SFXR_LOAD
#define AL_SFXR_SAVE
#define AL_SFXR_FLOAT_MONO
#include "../al_sfxr.h"
/*---------------------------------------------------------------------------*/

typedef struct {
    al_sfxr_Decoder* const decoder;
    int volatile playing;
}
ud_t;

static int fpreader(void* const userdata, uint8_t* const byte) {
    FILE* const fp = (FILE*)userdata;
    return fread(byte, 1, 1, fp) != 1;
}

static int load_sound(char const* const filename, al_sfxr_Params* const params) {
    FILE* const fp = fopen(filename, "rb");

    if (fp == NULL) {
        fprintf(stderr, "Error opening \"%s\": %s\n", filename, strerror(errno));
        return -1;
    }

    int const res = al_sfxr_load(params, fpreader, fp);
    fclose(fp);

    if (res != 0) {
        fprintf(stderr, "Error loading \"%s\": %s\n", filename, strerror(errno));
    }

    return res;
}

void data_callback(ma_device* const device, void* const output, const void* const input, ma_uint32 const frame_count) {
    (void)input;

    ud_t* const ud = (ud_t*)device->pUserData;
    size_t count = al_sfxr_produce1f(ud->decoder, output, frame_count);
    ud->playing = count != 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("No input file.\n");
        return EXIT_FAILURE;
    }

    al_sfxr_Params params;

    if (load_sound(argv[1], &params) != 0) {
        return EXIT_FAILURE;
    }

    al_sfxr_Decoder decoder;
    al_sfxr_start_quick(&decoder, &params);
    ud_t ud = {&decoder, 1};

    ma_device_config device_config;

    device_config = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format   = ma_format_f32;
    device_config.playback.channels = 1;
    device_config.sampleRate        = 44100;
    device_config.dataCallback      = data_callback;
    device_config.pUserData         = &ud;

    ma_device device;

    if (ma_device_init(NULL, &device_config, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        return EXIT_FAILURE;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to start playback device.\n");
        ma_device_uninit(&device);
        return EXIT_FAILURE;
    }

    while (ud.playing) /* nothing */;

    ma_device_uninit(&device);
    return EXIT_SUCCESS;
}
