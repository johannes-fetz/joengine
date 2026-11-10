/*
** Jo Engine ISO to BIN converter
** Copyright (c) 2026, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Converts the CD image made by mkisofs (game.iso: sectors of 2048 bytes, the data only) into a raw image of the
** data track (game.bin: sectors of 2352 bytes, CD-ROM mode 1), the format of a disc read sector by sector:
**
**   12 bytes   sync pattern (00 FF FF FF FF FF FF FF FF FF FF 00)
**    4 bytes   header: the address of the sector (minutes, seconds, frames in BCD, 2 seconds after the start of the
**              disc) and the mode (1)
** 2048 bytes   the data
**    4 bytes   EDC: CRC-32 of the sync, header and data
**    8 bytes   0
**  276 bytes   ECC: the parity bytes P (172) and Q (104), Reed-Solomon (ECMA-130, annex A)
**
** mkisofs can't write it: its option -sectype only writes sectors of 2048 (data) or 2056 (xa1) bytes.
**
** With a cue sheet, its data track becomes the new file: FILE "game.bin" BINARY and TRACK 01 MODE1/2352.
**
** Usage: JoEngineIsoToBin game.iso game.bin [game.cue]
** Build: cc -O2 -o JoEngineIsoToBin JoEngineIsoToBin.c (any C compiler: Windows, Linux, macOS)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ISO_SECTOR_SIZE         (2048)
#define RAW_SECTOR_SIZE         (2352)
// The data track starts 2 seconds (150 frames of 1/75 s) after the start of the disc
#define LEAD_IN_FRAMES          (150)

static unsigned int             edc_lut[256];
static unsigned char            ecc_f_lut[256];
static unsigned char            ecc_b_lut[256];

static void                     init_luts(void)
{
    unsigned int                i;
    unsigned int                j;
    unsigned int                edc;

    for (i = 0; i < 256; ++i)
    {
        // GF(2^8), x^8 + x^4 + x^3 + x^2 + 1: the multiplication by 2, and its inverse
        j = (i << 1) ^ ((i & 0x80) ? 0x11D : 0);
        ecc_f_lut[i] = (unsigned char)j;
        ecc_b_lut[i ^ j] = (unsigned char)i;
        // CRC-32 (x^32 + x^31 + x^16 + x^15 + x^4 + x^3 + x + 1), the bits from the lowest
        edc = i;
        for (j = 0; j < 8; ++j)
            edc = (edc >> 1) ^ ((edc & 1) ? 0xD8018001 : 0);
        edc_lut[i] = edc;
    }
}

static unsigned int             edc_compute(const unsigned char *src, size_t size)
{
    unsigned int                edc;

    edc = 0;
    while (size-- > 0)
        edc = (edc >> 8) ^ edc_lut[(edc ^ *src++) & 0xFF];
    return (edc);
}

/* One of the 2 parity blocks: major_count vectors of minor_count bytes (the bytes of the header from 12, the 16 bits
** words in 2 planes of bytes), 2 parity bytes per vector */
static void                     ecc_compute_block(const unsigned char *src, unsigned int major_count, unsigned int minor_count,
                                                  unsigned int major_mult, unsigned int minor_inc, unsigned char *dest)
{
    unsigned int                size;
    unsigned int                major;
    unsigned int                minor;
    unsigned int                index;
    unsigned char               ecc_a;
    unsigned char               ecc_b;
    unsigned char               temp;

    size = major_count * minor_count;
    for (major = 0; major < major_count; ++major)
    {
        index = (major >> 1) * major_mult + (major & 1);
        ecc_a = 0;
        ecc_b = 0;
        for (minor = 0; minor < minor_count; ++minor)
        {
            temp = src[index];
            index += minor_inc;
            if (index >= size)
                index -= size;
            ecc_a ^= temp;
            ecc_b ^= temp;
            ecc_a = ecc_f_lut[ecc_a];
        }
        ecc_a = ecc_b_lut[ecc_f_lut[ecc_a] ^ ecc_b];
        dest[major] = ecc_a;
        dest[major + major_count] = ecc_a ^ ecc_b;
    }
}

static unsigned char            to_bcd(unsigned int value)
{
    return ((unsigned char)(((value / 10) << 4) | (value % 10)));
}

static void                     build_sector(unsigned char *sector, unsigned int lba)
{
    unsigned int                address;
    unsigned int                edc;

    // Sync
    memset(sector, 0xFF, 12);
    sector[0] = 0;
    sector[11] = 0;
    // Header
    address = lba + LEAD_IN_FRAMES;
    sector[12] = to_bcd(address / (60 * 75));
    sector[13] = to_bcd((address / 75) % 60);
    sector[14] = to_bcd(address % 75);
    sector[15] = 1;
    // (the data: sector + 16, already there)
    edc = edc_compute(sector, 16 + ISO_SECTOR_SIZE);
    sector[0x810] = (unsigned char)edc;
    sector[0x811] = (unsigned char)(edc >> 8);
    sector[0x812] = (unsigned char)(edc >> 16);
    sector[0x813] = (unsigned char)(edc >> 24);
    memset(sector + 0x814, 0, 8);
    // P then Q (Q covers P)
    ecc_compute_block(sector + 0xC, 86, 24, 2, 86, sector + 0x81C);
    ecc_compute_block(sector + 0xC, 52, 43, 86, 88, sector + 0x8C8);
}

static const char               *file_name(const char *path)
{
    const char                  *name;

    name = path;
    for (; *path; ++path)
        if (*path == '/' || *path == '\\' || *path == ':')
            name = path + 1;
    return (name);
}

/* The data track of the cue sheet (the first FILE line and its TRACK line) becomes bin_path in MODE1/2352 (a cue
** sheet already changed stays the same). The other lines (the audio tracks) and the line endings are kept */
static int                      patch_cue(const char *cue_path, const char *bin_path)
{
    FILE                        *file;
    char                        *text;
    char                        *out;
    char                        *line;
    char                        *end;
    char                        *mode;
    long                        size;
    size_t                      length;
    int                         file_done;
    int                         mode_done;

    if ((file = fopen(cue_path, "rb")) == NULL)
    {
        fprintf(stderr, "JoEngineIsoToBin: can't read %s\n", cue_path);
        return (0);
    }
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    text = (char *)malloc((size_t)size + 1);
    out = (char *)malloc((size_t)size + strlen(bin_path) + 64);
    if (text == NULL || out == NULL || fread(text, 1, (size_t)size, file) != (size_t)size)
    {
        fclose(file);
        fprintf(stderr, "JoEngineIsoToBin: can't read %s\n", cue_path);
        return (0);
    }
    fclose(file);
    text[size] = '\0';
    out[0] = '\0';
    length = 0;
    file_done = 0;
    mode_done = 0;
    for (line = text; *line; line = end)
    {
        end = strchr(line, '\n');
        end = end == NULL ? line + strlen(line) : end + 1;
        if (!file_done && strncmp(line + strspn(line, " \t"), "FILE ", 5) == 0)
        {
            // FILE "game.bin" BINARY, the end of the line kept
            length += (size_t)sprintf(out + length, "FILE \"%s\" BINARY", file_name(bin_path));
            line += strcspn(line, "\r\n");
            file_done = 1;
        }
        else if (file_done && !mode_done && (mode = strstr(line, "MODE1/2048")) != NULL && mode < end)
        {
            memcpy(out + length, line, (size_t)(mode - line));
            length += (size_t)(mode - line);
            memcpy(out + length, "MODE1/2352", 10);
            length += 10;
            line = mode + 10;
            mode_done = 1;
        }
        else if (file_done && !mode_done && (mode = strstr(line, "MODE1/2352")) != NULL && mode < end)
            mode_done = 1;
        memcpy(out + length, line, (size_t)(end - line));
        length += (size_t)(end - line);
    }
    free(text);
    if (!file_done || !mode_done)
    {
        free(out);
        fprintf(stderr, "JoEngineIsoToBin: no data track (FILE and MODE1/2048) in %s\n", cue_path);
        return (0);
    }
    if ((file = fopen(cue_path, "wb")) == NULL || fwrite(out, 1, length, file) != length)
    {
        if (file != NULL)
            fclose(file);
        free(out);
        fprintf(stderr, "JoEngineIsoToBin: can't write %s\n", cue_path);
        return (0);
    }
    fclose(file);
    free(out);
    return (1);
}

int                             main(int argc, char **argv)
{
    FILE                        *input;
    FILE                        *output;
    unsigned char               sector[RAW_SECTOR_SIZE];
    unsigned int                lba;
    size_t                      read;

    if (argc != 3 && argc != 4)
    {
        fprintf(stderr, "Usage: JoEngineIsoToBin game.iso game.bin [game.cue]\n"
                        "Converts an ISO (sectors of 2048 bytes) into a raw image (sectors of 2352 bytes, MODE1/2352)\n"
                        "and changes the data track of the cue sheet to it.\n");
        return (1);
    }
    init_luts();
    // (the cue sheet first: the image is newer, make doesn't convert it again)
    if (argc == 4 && !patch_cue(argv[3], argv[2]))
        return (1);
    if ((input = fopen(argv[1], "rb")) == NULL)
    {
        fprintf(stderr, "JoEngineIsoToBin: can't read %s\n", argv[1]);
        return (1);
    }
    if ((output = fopen(argv[2], "wb")) == NULL)
    {
        fclose(input);
        fprintf(stderr, "JoEngineIsoToBin: can't write %s\n", argv[2]);
        return (1);
    }
    for (lba = 0; (read = fread(sector + 16, 1, ISO_SECTOR_SIZE, input)) > 0; ++lba)
    {
        // (the end of an ISO that is not a multiple of 2048 bytes: filled with 0)
        if (read < ISO_SECTOR_SIZE)
            memset(sector + 16 + read, 0, ISO_SECTOR_SIZE - read);
        build_sector(sector, lba);
        if (fwrite(sector, 1, RAW_SECTOR_SIZE, output) != RAW_SECTOR_SIZE)
        {
            fclose(input);
            fclose(output);
            fprintf(stderr, "JoEngineIsoToBin: can't write %s\n", argv[2]);
            return (1);
        }
    }
    fclose(input);
    if (fclose(output) != 0)
    {
        fprintf(stderr, "JoEngineIsoToBin: can't write %s\n", argv[2]);
        return (1);
    }
    printf("%s: %u sectors of 2352 bytes (MODE1/2352)\n", file_name(argv[2]), lba);
    return (0);
}
