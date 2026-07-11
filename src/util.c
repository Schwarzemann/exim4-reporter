#include "exim4_report.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

void trim(char *text)
{
    char *start = text;
    char *end;

    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';
}

void lowercase_ascii(char *text)
{
    while (*text) {
        *text = (char)tolower((unsigned char)*text);
        text++;
    }
}

const char *path_basename(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

int ensure_dir(const char *path)
{
    struct stat st;

    if (path == NULL || path[0] == '\0') {
        return -1;
    }

    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }

    if (errno != ENOENT) {
        return -1;
    }

    if (mkdir(path, 0755) == 0) {
        return 0;
    }

    return errno == EEXIST ? 0 : -1;
}

int ensure_parent_dirs(const char *path)
{
    char tmp[ER_MAX_PATH * 2];
    size_t len;

    if (path == NULL) {
        return -1;
    }

    len = strlen(path);
    if (len >= sizeof(tmp)) {
        return -1;
    }

    strcpy(tmp, path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (ensure_dir(tmp) != 0) {
                return -1;
            }
            *p = '/';
        }
    }

    return 0;
}
