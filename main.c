#include <stdint.h>
#include <stdlib.h>

#include "raylib.h"


#define BITS_IN_U64 (64)

#define XORSHIFT128_RAND_MAX ((uint32_t)0xFFFFFFFFu)
uint32_t xorshiftstate[4] = { 0x89583899, 0xa809fa8d, 0x23095280, 0x2f8d0298 };
uint32_t xorshift128(uint32_t *state);

int
main(void)
{
    const int fps = 30;
    const int pxsz = 2;
    const int scw = BITS_IN_U64 * 12 - 2 * pxsz;
    const int sch = BITS_IN_U64 *  8 - 2 * pxsz;
    const int pxw = BITS_IN_U64 * 12 / pxsz;
    const int pxh = BITS_IN_U64 *  8 / pxsz;
    const int row_words = pxw / BITS_IN_U64;
    const int len = (pxh * row_words);

    InitWindow(scw, sch, "Conway's Game of Life");
    SetTargetFPS(fps);

    uint64_t *buf = malloc(2 * len * sizeof(*buf));
    uint64_t *cur = buf;
    uint64_t *prv = &buf[len];

    for (int i = 0; i < len; i++) {
        cur[i] = (uint64_t)xorshift128(xorshiftstate) |
                 (uint64_t)xorshift128(xorshiftstate) << 32;
        prv[i] = cur[i];
    }

    // bitcount lut
    uint8_t lut[8] = {
        0, // 000
        1, // 001
        1, // 010
        2, // 011
        1, // 100
        2, // 101
        2, // 110
        3, // 111
    };

    while (!WindowShouldClose()) {

        uint64_t topw, midw, botw, word;
        int topi, midi, boti, bitcount;
        uint8_t live, next;
        for (int j = 1; j < pxh - 1; j++) {

            topi = (j - 1) * row_words;
            midi = j * row_words;
            boti = (j + 1) * row_words;

            for (int i = 0; i < row_words; i++) {
                topw = prv[topi + i];
                midw = prv[midi + i];
                botw = prv[boti + i];
                word = 0;

                // left border between words
                if (i > 0) {
                    bitcount = lut[(topw >> (BITS_IN_U64 - 2)) & 0x3] + // 11
                               lut[(midw >> (BITS_IN_U64 - 2)) & 0x1] + // 01
                               lut[(botw >> (BITS_IN_U64 - 2)) & 0x3] + // 11
                               (prv[topi + i - 1] & 1) + // 1
                               (prv[midi + i - 1] & 1) + // 1
                               (prv[boti + i - 1] & 1);  // 1
                    live = (midw >> (BITS_IN_U64 - 1)) & 1; // left-most middle bit
                    next = (bitcount == 3) || ((bitcount == 2) && live);
                    word |= ((uint64_t)next & 1) << (BITS_IN_U64 - 1);
                }

                // right border between words
                if (i < (row_words - 1)) {
                    bitcount = lut[topw & 0x3] + // 11
                               lut[midw & 0x1] + // 01
                               lut[botw & 0x3] + // 11
                               ((prv[topi + i + 1] >> (BITS_IN_U64 - 1)) & 1) + // 1
                               ((prv[midi + i + 1] >> (BITS_IN_U64 - 1)) & 1) + // 1
                               ((prv[boti + i + 1] >> (BITS_IN_U64 - 1)) & 1);  // 1
                    live = midw & 1;
                    next = (bitcount == 3) || ((bitcount == 2) && live);
                    word |= (uint64_t)next & 1;
                }

                // within word
                for (int bit = 1; bit < BITS_IN_U64 - 1; bit++) {
                    bitcount = lut[(topw >> (bit - 1)) & 0x7] + // 111
                               lut[(midw >> (bit - 1)) & 0x5] + // 101
                               lut[(botw >> (bit - 1)) & 0x7];  // 111
                    live = (midw >> bit) & 1; // current cell
                    next = (bitcount == 3) || ((bitcount == 2) && live);
                    word |= ((uint64_t)next & 1) << bit;
                }

                cur[midi + i] = word;
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        for (int j = 1; j < pxh-1; j++) {
            for (int i = 1; i < pxw-1; i++) {
                int idx = j * row_words + i / BITS_IN_U64;
                int bit = i & (BITS_IN_U64 - 1);
                if ((cur[idx] >> bit) & 1)
                    DrawRectangle((i - 1) * pxsz, (j - 1) * pxsz, pxsz, pxsz, RAYWHITE);
            }
        }

        EndDrawing();

        // swap
        uint64_t *tmp = prv; prv = cur; cur = tmp;
    }

    free(buf);

    CloseWindow();

    return 0;
}

uint32_t
xorshift128(uint32_t *state)
{
    uint32_t t, s;
    t = state[3];
    s = state[0];
    state[3] = state[2];
    state[2] = state[1];
    state[1] = s;
    t ^= t << 11;
    t ^= t >> 8;
    state[0] = t ^ s ^ (s >> 19);
    return state[0];
}

