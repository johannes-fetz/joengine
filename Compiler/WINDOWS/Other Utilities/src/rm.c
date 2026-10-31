/*
** rm.exe for the Jo Engine (Windows): native replacement of the MSYS rm.exe
**
** The MSYS rm.exe needs msys-1.0.dll, which sometimes fails to start:
** "Couldn't reserve space for cygwin's heap, Win32 error 0" (the address it needs is taken).
** This one only uses the Windows API (no DLL to install, no fixed address).
**
** Usage: rm [-r] [-R] [-f] [-v] [--] FILE...
**   -r, -R   remove the directories and their contents
**   -f       ignore the missing files, no error
**   -v       print each removed file
**   FILE may contain the wildcards * and ? (in the last part of the path), / or \ as separator
**
** Build (Linux, package gcc-mingw-w64-i686): i686-w64-mingw32-gcc -O2 -s -municode -static -o rm.exe rm.c
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

static int  recursive = 0;
static int  force = 0;
static int  verbose = 0;
static int  errors = 0;

static void report(const wchar_t *path, DWORD error)
{
    wchar_t msg[512];

    if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, 0, msg, 512, NULL))
        swprintf(msg, 512, L"error %lu\n", (unsigned long)error);
    fwprintf(stderr, L"rm: %ls: %ls", path, msg);
    ++errors;
}

/* A file locked for a moment (antivirus, indexer, the previous build) is deleted after a few tries */
static BOOL retry(BOOL (*action)(const wchar_t *), const wchar_t *path)
{
    int     i;
    DWORD   e;

    for (i = 0; i < 20; ++i)
    {
        if (action(path))
            return (TRUE);
        e = GetLastError();
        if (e != ERROR_SHARING_VIOLATION && e != ERROR_ACCESS_DENIED && e != ERROR_DIR_NOT_EMPTY && e != ERROR_LOCK_VIOLATION)
            return (FALSE);
        Sleep(50);
    }
    return (FALSE);
}

static BOOL delete_file(const wchar_t *path)
{
    return (DeleteFileW(path));
}

static BOOL delete_dir(const wchar_t *path)
{
    return (RemoveDirectoryW(path));
}

static void remove_path(const wchar_t *path);

static void remove_dir_contents(const wchar_t *dir)
{
    wchar_t             pattern[MAX_PATH * 2];
    wchar_t             child[MAX_PATH * 2];
    WIN32_FIND_DATAW    data;
    HANDLE              h;

    swprintf(pattern, MAX_PATH * 2, L"%ls\\*", dir);
    h = FindFirstFileW(pattern, &data);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do
    {
        if (!wcscmp(data.cFileName, L".") || !wcscmp(data.cFileName, L".."))
            continue;
        swprintf(child, MAX_PATH * 2, L"%ls\\%ls", dir, data.cFileName);
        remove_path(child);
    } while (FindNextFileW(h, &data));
    FindClose(h);
}

static void remove_path(const wchar_t *path)
{
    DWORD   attributes;

    attributes = GetFileAttributesW(path);
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        if (!force)
            report(path, GetLastError());
        return;
    }
    // Read only files are removed too (like rm -f)
    if (attributes & FILE_ATTRIBUTE_READONLY)
        SetFileAttributesW(path, attributes & ~FILE_ATTRIBUTE_READONLY);
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (!recursive)
        {
            fwprintf(stderr, L"rm: %ls: is a directory\n", path);
            ++errors;
            return;
        }
        // A link to a directory (junction) is removed, not what it points to
        if (!(attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            remove_dir_contents(path);
        if (!retry(delete_dir, path))
            report(path, GetLastError());
        else if (verbose)
            wprintf(L"removed directory '%ls'\n", path);
        return;
    }
    if (!retry(delete_file, path))
        report(path, GetLastError());
    else if (verbose)
        wprintf(L"removed '%ls'\n", path);
}

static void remove_argument(wchar_t *arg)
{
    wchar_t             *p;
    wchar_t             *name;
    wchar_t             child[MAX_PATH * 2];
    WIN32_FIND_DATAW    data;
    HANDLE              h;
    size_t              dir_length;
    int                 found;

    for (p = arg; *p; ++p)
        if (*p == L'/')
            *p = L'\\';
    // Trailing separators: "dir/" is "dir"
    while (p > arg + 1 && p[-1] == L'\\' && p[-2] != L':')
        *--p = L'\0';
    if (!wcspbrk(arg, L"*?"))
    {
        remove_path(arg);
        return;
    }
    name = wcsrchr(arg, L'\\');
    dir_length = name ? (size_t)(name - arg + 1) : 0;
    found = 0;
    h = FindFirstFileW(arg, &data);
    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!wcscmp(data.cFileName, L".") || !wcscmp(data.cFileName, L".."))
                continue;
            swprintf(child, MAX_PATH * 2, L"%.*ls%ls", (int)dir_length, arg, data.cFileName);
            remove_path(child);
            found = 1;
        } while (FindNextFileW(h, &data));
        FindClose(h);
    }
    if (!found && !force)
    {
        fwprintf(stderr, L"rm: %ls: No such file or directory\n", arg);
        ++errors;
    }
}

int wmain(int argc, wchar_t **argv)
{
    int     i;
    int     options;
    int     operands;
    wchar_t *o;

    options = 1;
    operands = 0;
    for (i = 1; i < argc; ++i)
    {
        if (options && argv[i][0] == L'-' && argv[i][1] != L'\0')
        {
            if (!wcscmp(argv[i], L"--"))
            {
                options = 0;
                continue;
            }
            if (!wcscmp(argv[i], L"--recursive"))
                recursive = 1;
            else if (!wcscmp(argv[i], L"--force"))
                force = 1;
            else if (!wcscmp(argv[i], L"--verbose"))
                verbose = 1;
            else
                for (o = argv[i] + 1; *o; ++o)
                {
                    if (*o == L'r' || *o == L'R')
                        recursive = 1;
                    else if (*o == L'f')
                        force = 1;
                    else if (*o == L'v')
                        verbose = 1;
                    else if (*o != L'd' && *o != L'i' && *o != L'I')
                    {
                        fwprintf(stderr, L"rm: unknown option -%lc\n", *o);
                        return (1);
                    }
                }
            continue;
        }
        ++operands;
        remove_argument(argv[i]);
    }
    if (!operands && !force)
    {
        fwprintf(stderr, L"usage: rm [-rf] [-v] FILE...\n");
        return (1);
    }
    return (errors ? 1 : 0);
}
