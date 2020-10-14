#include <stdio.h>
#include <inttypes.h>

/*---------------------------------------------------------------------------*/
/* stb_image config and inclusion */

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#define STBI_ASSERT(x)
#include <stb_image.h>
/*---------------------------------------------------------------------------*/

int main(int const argc, char const* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: imgconv <filename> <id>\n");
        return EXIT_FAILURE;
    }

    int width, height;
    uint32_t const* abgr = (uint32_t*)stbi_load(argv[1], &width, &height, NULL, STBI_rgb_alpha);
    uint32_t const* abgr_orig = abgr;

    if (abgr == NULL) {
        fprintf(stderr, "Error loading image: %s\n", stbi_failure_reason());
        return EXIT_FAILURE;
    }

    printf("static int const %s_width = %d;\n", argv[2], width);
    printf("static int const %s_height = %d;\n\n", argv[2], height);
    printf("static uint32_t const %s_abgr[] = {\n", argv[2]);

    int col = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++, abgr++) {
            if (col == 0) {
                printf("    ");
            }

            printf("0x%08" PRIx32 ", ", *abgr);

            if (++col == 8) {
                col = 0;
                printf("\n");
            }
        }
    }

    printf("};\n");
    stbi_image_free((void*)abgr_orig);
    return EXIT_SUCCESS;
}
