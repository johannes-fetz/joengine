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
** With a cue sheet (made by JoEngineCueMaker), its audio tracks are put after the data track in game.bin too (WAV:
** PCM, 44100 Hz, 16 bits, stereo; BINARY: raw CD audio), each one padded to a whole sector (4 seconds at least, the
** shortest track of a CD), and game.cue becomes the cue sheet of this single file (TRACK 01 MODE1/2352, each audio track
** at its position: INDEX 01), the gaps kept (PREGAP, POSTGAP: not in the file). A single BIN/CUE: what the burning
** tools and the emulators read. An MP3 track can't be put in it (a CD can't hold it): convert it to WAV.
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

static unsigned int             read_le(const unsigned char *p, int size)
{
    unsigned int                value;

    value = 0;
    while (size-- > 0)
        value = (value << 8) | p[size];
    return (value);
}

/*
** The cue sheet
*/

#define MAX_TRACKS              (99)
#define MAX_LINE                (1024)

typedef struct
{
    char                        path[MAX_LINE];         // The file of the track, as in the cue sheet
    char                        type[16];               // BINARY, WAVE, MP3...
    char                        mode[16];               // MODE1/2048, AUDIO...
    int                         number;                 // TRACK
    int                         pregap;                 // Frames (1/75 s), -1: none
    int                         postgap;                // Frames, -1: none
    unsigned int                start;                  // Its first sector in game.bin
}                               cue_track;

static cue_track                tracks[MAX_TRACKS];
static int                      track_count;

static int                      parse_msf(const char *text, int *frames)
{
    unsigned int                m;
    unsigned int                s;
    unsigned int                f;

    if (sscanf(text, "%u:%u:%u", &m, &s, &f) != 3 || s >= 60 || f >= 75)
        return (0);
    *frames = (int)((m * 60 + s) * 75 + f);
    return (1);
}

static void                     print_msf(FILE *file, unsigned int frames)
{
    fprintf(file, "%02u:%02u:%02u", frames / (60 * 75), (frames / 75) % 60, frames % 75);
}

/* The cue sheet of JoEngineCueMaker: a FILE for each track, its TRACK, INDEX 01 00:00:00 and the gaps (PREGAP, POSTGAP) */
static int                      read_cue(const char *cue_path)
{
    FILE                        *file;
    char                        line[MAX_LINE];
    char                        word[MAX_LINE];
    char                        *p;
    char                        *quote;
    cue_track                   *track;
    int                         line_number;
    int                         number;
    int                         frames;

    if ((file = fopen(cue_path, "r")) == NULL)
    {
        fprintf(stderr, "JoEngineIsoToBin: can't read %s\n", cue_path);
        return (0);
    }
    track_count = 0;
    track = NULL;
    line_number = 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        ++line_number;
        for (p = line + strlen(line); p > line && (p[-1] == '\n' || p[-1] == '\r' || p[-1] == ' ' || p[-1] == '\t'); --p)
            p[-1] = '\0';
        p = line + strspn(line, " \t");
        if (*p == '\0' || sscanf(p, "%1023s", word) != 1)
            continue;
        if (strcmp(word, "FILE") == 0)
        {
            if (track_count == MAX_TRACKS)
                goto unsupported;
            track = &tracks[track_count++];
            memset(track, 0, sizeof(*track));
            track->pregap = -1;
            track->postgap = -1;
            p += 4;
            p += strspn(p, " \t");
            if (*p == '"' && (quote = strrchr(p + 1, '"')) != NULL)
            {
                memcpy(track->path, p + 1, (size_t)(quote - p - 1));
                track->path[quote - p - 1] = '\0';
                p = quote + 1;
            }
            else if (sscanf(p, "%1023s", track->path) == 1)
                p += strlen(track->path);
            if (track->path[0] == '\0' || sscanf(p, "%15s", track->type) != 1)
                goto unsupported;
        }
        else if (strcmp(word, "TRACK") == 0)
        {
            // (one track per file)
            if (track == NULL || track->number != 0 || sscanf(p + 5, "%d %15s", &track->number, track->mode) != 2)
                goto unsupported;
        }
        else if (strcmp(word, "INDEX") == 0)
        {
            // (INDEX 01 at the start of the file only)
            if (track == NULL || track->number == 0 || sscanf(p + 5, "%d %1023s", &number, word) != 2 || number != 1 ||
                !parse_msf(word, &frames) || frames != 0)
                goto unsupported;
        }
        else if (strcmp(word, "PREGAP") == 0 || strcmp(word, "POSTGAP") == 0)
        {
            if (track == NULL || track->number == 0 || !parse_msf(p + strlen(word) + strspn(p + strlen(word), " \t"), &frames))
                goto unsupported;
            if (word[1] == 'R')
                track->pregap = frames;
            else
                track->postgap = frames;
        }
        // (REM, TITLE, PERFORMER...: ignored)
    }
    fclose(file);
    if (track_count == 0 || tracks[0].number == 0)
    {
        fprintf(stderr, "JoEngineIsoToBin: no data track in %s\n", cue_path);
        return (0);
    }
    for (number = 0; number < track_count; ++number)
        if (tracks[number].number == 0)
        {
            fprintf(stderr, "JoEngineIsoToBin: no TRACK for %s in %s\n", tracks[number].path, cue_path);
            return (0);
        }
    return (1);
unsupported:
    fclose(file);
    fprintf(stderr, "JoEngineIsoToBin: %s, line %d: not a cue sheet of JoEngineCueMaker (one track per FILE, its INDEX 01 at "
                    "00:00:00; already converted?)\n", cue_path, line_number);
    return (0);
}

/* The path of a file of the cue sheet: relative to the folder of the cue sheet */
static void                     track_path(const char *cue_path, const char *path, char *out, size_t size)
{
    size_t                      folder;

    folder = (size_t)(file_name(cue_path) - cue_path);
    if (path[0] == '/' || path[0] == '\\' || (path[0] != '\0' && path[1] == ':') || folder == 0 || folder + strlen(path) >= size)
        snprintf(out, size, "%s", path);
    else
    {
        memcpy(out, cue_path, folder);
        strcpy(out + folder, path);
    }
}

/* The PCM data of a WAV file in the format of a CD (44100 Hz, 16 bits, stereo): the file at the start of its data, its size */
static FILE                     *open_wav(const char *path, unsigned int *data_size)
{
    FILE                        *file;
    unsigned char               header[12];
    unsigned char               chunk[8];
    unsigned char               format[16];
    unsigned int                size;
    int                         format_ok;

    if ((file = fopen(path, "rb")) == NULL)
    {
        fprintf(stderr, "JoEngineIsoToBin: can't read %s\n", path);
        return (NULL);
    }
    format_ok = 0;
    if (fread(header, 1, 12, file) != 12 || memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0)
    {
        fclose(file);
        fprintf(stderr, "JoEngineIsoToBin: %s is not a WAV file\n", path);
        return (NULL);
    }
    while (fread(chunk, 1, 8, file) == 8)
    {
        size = read_le(chunk + 4, 4);
        if (memcmp(chunk, "fmt ", 4) == 0 && size >= 16)
        {
            if (fread(format, 1, 16, file) != 16)
                break;
            if (read_le(format, 2) != 1 || read_le(format + 2, 2) != 2 || read_le(format + 4, 4) != 44100 ||
                read_le(format + 14, 2) != 16)
            {
                fclose(file);
                fprintf(stderr, "JoEngineIsoToBin: %s is not a CD audio WAV (PCM, 44100 Hz, 16 bits, stereo): %u Hz, "
                                "%u bits, %u channels\n", path, read_le(format + 4, 4), read_le(format + 14, 2),
                                read_le(format + 2, 2));
                return (NULL);
            }
            format_ok = 1;
            size -= 16;
        }
        else if (memcmp(chunk, "data", 4) == 0 && format_ok)
        {
            *data_size = size;
            return (file);
        }
        if (fseek(file, (long)(size + (size & 1)), SEEK_CUR) != 0)
            break;
    }
    fclose(file);
    fprintf(stderr, "JoEngineIsoToBin: %s: no PCM data (fmt and data chunks)\n", path);
    return (NULL);
}

/* An audio track appended to game.bin, padded to a whole sector: its number of sectors (0: failed) */
static unsigned int             append_audio(FILE *output, const char *cue_path, const cue_track *track)
{
    char                        path[MAX_LINE * 2];
    unsigned char               buffer[RAW_SECTOR_SIZE * 16];
    FILE                        *input;
    unsigned int                remaining;
    unsigned long               written;
    size_t                      chunk;
    size_t                      read;

    track_path(cue_path, track->path, path, sizeof(path));
    remaining = 0xFFFFFFFF;
    if (strcmp(track->type, "WAVE") == 0)
    {
        if ((input = open_wav(path, &remaining)) == NULL)
            return (0);
    }
    else if (strcmp(track->type, "BINARY") == 0)
    {
        if ((input = fopen(path, "rb")) == NULL)
        {
            fprintf(stderr, "JoEngineIsoToBin: can't read %s\n", path);
            return (0);
        }
    }
    else
    {
        fprintf(stderr, "JoEngineIsoToBin: %s: a %s track can't be put in the image (a CD only holds raw audio): convert "
                        "it to WAV (44100 Hz, 16 bits, stereo)\n", path, track->type);
        return (0);
    }
    written = 0;
    while (remaining > 0)
    {
        chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        if ((read = fread(buffer, 1, chunk, input)) == 0)
            break;
        if (fwrite(buffer, 1, read, output) != read)
        {
            fclose(input);
            return (0);
        }
        written += read;
        if (remaining != 0xFFFFFFFF)
            remaining -= (unsigned int)read;
    }
    fclose(input);
    // (a track of at least 4 seconds, the shortest track of a CD; padded to a whole sector with silence)
    chunk = (size_t)((RAW_SECTOR_SIZE - written % RAW_SECTOR_SIZE) % RAW_SECTOR_SIZE);
    if (written + chunk < (unsigned long)RAW_SECTOR_SIZE * 300)
        chunk = (size_t)((unsigned long)RAW_SECTOR_SIZE * 300 - written);
    memset(buffer, 0, sizeof(buffer));
    written += chunk;
    while (chunk > 0)
    {
        read = chunk < sizeof(buffer) ? chunk : sizeof(buffer);
        if (fwrite(buffer, 1, read, output) != read)
            return (0);
        chunk -= read;
    }
    return ((unsigned int)(written / RAW_SECTOR_SIZE));
}

/* The cue sheet of the single image: each track at its position in it, the gaps kept */
static int                      write_cue(const char *cue_path, const char *bin_path)
{
    FILE                        *file;
    int                         i;

    if ((file = fopen(cue_path, "w")) == NULL)
    {
        fprintf(stderr, "JoEngineIsoToBin: can't write %s\n", cue_path);
        return (0);
    }
    fprintf(file, "FILE \"%s\" BINARY\n", file_name(bin_path));
    for (i = 0; i < track_count; ++i)
    {
        fprintf(file, "  TRACK %02d %s\n", tracks[i].number, i == 0 ? "MODE1/2352" : tracks[i].mode);
        if (tracks[i].pregap >= 0)
        {
            fprintf(file, "    PREGAP ");
            print_msf(file, (unsigned int)tracks[i].pregap);
            fprintf(file, "\n");
        }
        fprintf(file, "    INDEX 01 ");
        print_msf(file, tracks[i].start);
        fprintf(file, "\n");
        if (tracks[i].postgap >= 0)
        {
            fprintf(file, "    POSTGAP ");
            print_msf(file, (unsigned int)tracks[i].postgap);
            fprintf(file, "\n");
        }
    }
    if (fclose(file) != 0)
    {
        fprintf(stderr, "JoEngineIsoToBin: can't write %s\n", cue_path);
        return (0);
    }
    return (1);
}

int                             main(int argc, char **argv)
{
    FILE                        *input;
    FILE                        *output;
    unsigned char               sector[RAW_SECTOR_SIZE];
    unsigned int                lba;
    unsigned int                sectors;
    size_t                      read;
    int                         i;

    if (argc != 3 && argc != 4)
    {
        fprintf(stderr, "Usage: JoEngineIsoToBin game.iso game.bin [game.cue]\n"
                        "Converts an ISO (sectors of 2048 bytes) into a raw image (sectors of 2352 bytes, MODE1/2352).\n"
                        "With the cue sheet: its audio tracks are put in the image too, and the cue sheet describes it.\n");
        return (1);
    }
    init_luts();
    if (argc == 4)
    {
        if (!read_cue(argv[3]))
            return (1);
        if (strcmp(file_name(tracks[0].path), file_name(argv[2])) == 0)
        {
            fprintf(stderr, "JoEngineIsoToBin: %s already describes %s\n", argv[3], file_name(argv[2]));
            return (1);
        }
    }
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
    printf("%s: track 1, data: %u sectors of 2352 bytes (MODE1/2352)\n", file_name(argv[2]), lba);
    sectors = lba;
    for (i = 1; argc == 4 && i < track_count; ++i)
    {
        tracks[i].start = sectors;
        if ((lba = append_audio(output, argv[3], &tracks[i])) == 0)
        {
            fclose(output);
            remove(argv[2]);
            return (1);
        }
        sectors += lba;
        printf("%s: track %d, audio: %s, %u.%02u s\n", file_name(argv[2]), tracks[i].number, file_name(tracks[i].path),
               lba / 75, (lba % 75) * 100 / 75);
    }
    if (fclose(output) != 0)
    {
        fprintf(stderr, "JoEngineIsoToBin: can't write %s\n", argv[2]);
        return (1);
    }
    if (argc == 4 && !write_cue(argv[3], argv[2]))
        return (1);
    return (0);
}
