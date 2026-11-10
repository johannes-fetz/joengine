/*
** Jo Engine Cue Sheet Maker
** Copyright (c) 2020-2026, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Makes game.cue: the data track (game.iso, track 1), then the audio files of the audio folder (the folder of the game
** by default) as the audio tracks 2, 3, etc. in alphabetical order (the case ignored: the same order on Windows, Linux
** and macOS):
**   *.wav, *.wave   WAVE (PCM, 44100 Hz, 16 bits, stereo: the format of a CD; a warning otherwise)
**   *.bin           BINARY (raw CD audio: 44100 Hz, 16 bits, stereo, little endian), except game.ip.bin and game.bin
**   *.mp3           MP3 (some emulators only: a CD can't hold it)
** The data track is followed by 2 seconds of gap (POSTGAP), and the first audio track by 2 seconds (PREGAP).
** jo_audio_play_cd_track(2, 2, true) plays the first audio file.
**
** Usage: JoEngineCueMaker [game.iso [audio folder]]
**        (without argument: the only *.iso of the current folder, the audio files of the current folder)
** Build: cc -O2 -o JoEngineCueMaker JoEngineCueMaker.c (any C compiler: Windows, Linux, macOS)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#define MAX_PATH_LENGTH         (1024)

typedef struct
{
    char                        *name;
    const char                  *type;
}                               audio_file;

static int                      ends_with(const char *name, const char *suffix)
{
    size_t                      name_length;
    size_t                      suffix_length;
    size_t                      i;

    name_length = strlen(name);
    suffix_length = strlen(suffix);
    if (name_length < suffix_length)
        return (0);
    for (i = 0; i < suffix_length; ++i)
        if (tolower((unsigned char)name[name_length - suffix_length + i]) != tolower((unsigned char)suffix[i]))
            return (0);
    return (1);
}

/* The alphabetical order of the tracks: the case ignored, then the case (a stable order for names that differ only by it) */
static int                      compare_audio_files(const void *a, const void *b)
{
    const char                  *name_a;
    const char                  *name_b;
    int                         ca;
    int                         cb;
    size_t                      i;

    name_a = ((const audio_file *)a)->name;
    name_b = ((const audio_file *)b)->name;
    for (i = 0; name_a[i] || name_b[i]; ++i)
    {
        ca = toupper((unsigned char)name_a[i]);
        cb = toupper((unsigned char)name_b[i]);
        if (ca != cb)
            return (ca - cb);
    }
    return (strcmp(name_a, name_b));
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

/* A warning when a WAV file is not in the format of a CD (44100 Hz, 16 bits, stereo, PCM) */
static void                     check_wav(const char *path)
{
    FILE                        *file;
    unsigned char               header[12];
    unsigned char               chunk[8];
    unsigned char               format[16];
    unsigned int                size;

    if ((file = fopen(path, "rb")) == NULL)
        return;
    if (fread(header, 1, 12, file) != 12 || memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0)
    {
        fclose(file);
        fprintf(stderr, "Warning: %s is not a WAV file\n", path);
        return;
    }
    while (fread(chunk, 1, 8, file) == 8)
    {
        size = read_le(chunk + 4, 4);
        if (memcmp(chunk, "fmt ", 4) == 0 && size >= 16 && fread(format, 1, 16, file) == 16)
        {
            if (read_le(format, 2) != 1 || read_le(format + 2, 2) != 2 || read_le(format + 4, 4) != 44100 ||
                read_le(format + 14, 2) != 16)
                fprintf(stderr, "Warning: %s is not a CD audio WAV (PCM, 44100 Hz, 16 bits, stereo): %u Hz, %u bits, "
                                "%u channels\n", path, read_le(format + 4, 4), read_le(format + 14, 2), read_le(format + 2, 2));
            fclose(file);
            return;
        }
        if (fseek(file, (long)(size + (size & 1)), SEEK_CUR) != 0)
            break;
    }
    fclose(file);
    fprintf(stderr, "Warning: %s has no format (fmt chunk)\n", path);
}

/* The only *.iso of the current folder */
static int                      find_iso(char *iso, size_t size)
{
    DIR                         *dir;
    struct dirent               *entry;
    int                         count;

    if ((dir = opendir(".")) == NULL)
    {
        fprintf(stderr, "Failed to open the current folder\n");
        return (0);
    }
    count = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        if (!ends_with(entry->d_name, ".iso"))
            continue;
        if (++count == 1 && strlen(entry->d_name) < size)
            strcpy(iso, entry->d_name);
    }
    closedir(dir);
    if (count != 1)
    {
        fprintf(stderr, count == 0 ? "No ISO found in the current folder\n" : "Multiple ISO found in the current folder\n");
        return (0);
    }
    return (1);
}

int                             main(int argc, char **argv)
{
    char                        iso[MAX_PATH_LENGTH];
    char                        cue_path[MAX_PATH_LENGTH];
    char                        raw_image[MAX_PATH_LENGTH];
    char                        audio_dir[MAX_PATH_LENGTH];
    char                        path[MAX_PATH_LENGTH * 2];
    const char                  *type;
    audio_file                  *files;
    size_t                      count;
    size_t                      capacity;
    size_t                      length;
    size_t                      i;
    DIR                         *dir;
    struct dirent               *entry;
    FILE                        *cue;
    char                        *dot;
    int                         track;

    printf("Jo Engine Cue Sheet Maker v2.0 (c) 2020-2026 by Johannes Fetz\n\n");
    if (argc > 3 || (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)))
    {
        fprintf(stderr, "Usage: JoEngineCueMaker [game.iso [audio folder]]\n");
        return (1);
    }
    if (argc > 1)
    {
        if (strlen(argv[1]) >= sizeof(iso))
            return (1);
        strcpy(iso, argv[1]);
    }
    else if (!find_iso(iso, sizeof(iso)))
        return (1);
    // (the audio folder as written in the cue sheet: / separators, no trailing separator, "." for the current folder)
    strcpy(audio_dir, ".");
    if (argc > 2 && argv[2][0] != '\0')
    {
        if (strlen(argv[2]) >= sizeof(audio_dir))
            return (1);
        strcpy(audio_dir, argv[2]);
        for (i = 0; audio_dir[i]; ++i)
            if (audio_dir[i] == '\\')
                audio_dir[i] = '/';
        length = strlen(audio_dir);
        while (length > 1 && audio_dir[length - 1] == '/')
            audio_dir[--length] = '\0';
    }
    printf("%s found.\n", file_name(iso));
    // game.cue and game.bin (the raw image of JO_CD_SECTOR_SIZE = 2352, not a track) next to game.iso
    strcpy(cue_path, iso);
    if ((dot = strrchr(cue_path, '.')) != NULL && dot > file_name(cue_path))
        *dot = '\0';
    strcpy(raw_image, file_name(cue_path));
    strcat(raw_image, ".bin");
    strcat(cue_path, ".cue");
    // The audio files
    if ((dir = opendir(audio_dir)) == NULL)
    {
        fprintf(stderr, "Failed to open the audio folder %s\n", audio_dir);
        return (1);
    }
    files = NULL;
    count = 0;
    capacity = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        if (ends_with(entry->d_name, ".wav") || ends_with(entry->d_name, ".wave"))
            type = "WAVE";
        else if (ends_with(entry->d_name, ".mp3"))
            type = "MP3";
        else if (ends_with(entry->d_name, ".bin") && !ends_with(entry->d_name, ".ip.bin") &&
                 !(strcmp(audio_dir, ".") == 0 && strlen(entry->d_name) == strlen(raw_image) && ends_with(entry->d_name, raw_image)))
            type = "BINARY";
        else
            continue;
        if (count == capacity)
        {
            capacity = capacity ? capacity * 2 : 16;
            if ((files = (audio_file *)realloc(files, capacity * sizeof(*files))) == NULL)
                return (1);
        }
        if ((files[count].name = (char *)malloc(strlen(entry->d_name) + 1)) == NULL)
            return (1);
        strcpy(files[count].name, entry->d_name);
        files[count].type = type;
        ++count;
    }
    closedir(dir);
    if (count > 0)
        qsort(files, count, sizeof(*files), compare_audio_files);
    if (count > 98)
    {
        fprintf(stderr, "Too many audio tracks: %u (a CD has 99 tracks, the first is the data)\n", (unsigned int)count);
        return (1);
    }
    // The cue sheet
    if ((cue = fopen(cue_path, "w")) == NULL)
    {
        fprintf(stderr, "Could not open %s for writing\n", cue_path);
        return (1);
    }
    fprintf(cue, "FILE \"%s\" BINARY\n", file_name(iso));
    fprintf(cue, "  TRACK 01 MODE1/2048\n");
    fprintf(cue, "    INDEX 01 00:00:00\n");
    fprintf(cue, "    POSTGAP 00:02:00\n");
    track = 2;
    for (i = 0; i < count; ++i)
    {
        if (strcmp(audio_dir, ".") == 0)
            strcpy(path, files[i].name);
        else
            sprintf(path, "%s/%s", audio_dir, files[i].name);
        printf("%s found: track %d.\n", path, track);
        if (strcmp(files[i].type, "WAVE") == 0)
            check_wav(path);
        fprintf(cue, "FILE \"%s\" %s\n", path, files[i].type);
        fprintf(cue, "  TRACK %02d AUDIO\n", track);
        // (the gap after the data track: before the first audio track only)
        if (track == 2)
            fprintf(cue, "    PREGAP 00:02:00\n");
        fprintf(cue, "    INDEX 01 00:00:00\n");
        ++track;
        free(files[i].name);
    }
    free(files);
    if (fclose(cue) != 0)
    {
        fprintf(stderr, "Could not write %s\n", cue_path);
        return (1);
    }
    printf("\n%s generated.\n", file_name(cue_path));
    return (0);
}
