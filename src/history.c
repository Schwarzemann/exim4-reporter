#include "exim4_report.h"

#include <stdio.h>
#include <string.h>

static int history_path(const char *root, const char *domain_name, char *path, size_t size)
{
    int n = snprintf(path, size, "%s/%s/history.csv", root, domain_name);
    return n > 0 && (size_t)n < size ? 0 : -1;
}

int update_history(const char *root, const char *domain_name, ReportDate date, Totals totals)
{
    char path[ER_MAX_PATH * 2];
    char temp_path[ER_MAX_PATH * 2];
    char line[512];
    char wanted[16];
    int replaced = 0;
    FILE *in;
    FILE *fp;

    if (history_path(root, domain_name, path, sizeof(path)) != 0) {
        return -1;
    }
    if (ensure_parent_dirs(path) != 0) {
        return -1;
    }
    if (snprintf(temp_path, sizeof(temp_path), "%s.tmp", path) >= (int)sizeof(temp_path)) {
        return -1;
    }

    snprintf(wanted, sizeof(wanted), "%04d-%02d-%02d", date.year, date.month, date.day);

    fp = fopen(temp_path, "w");
    if (fp == NULL) {
        perror(temp_path);
        return -1;
    }

    in = fopen(path, "r");
    if (in != NULL) {
        while (fgets(line, sizeof(line), in) != NULL) {
            if (strncmp(line, wanted, 10) == 0 && line[10] == ',') {
                if (!replaced) {
                    fprintf(fp, "%s,%lu,%lu,%llu\n",
                            wanted, totals.sent, totals.received, totals.bytes);
                    replaced = 1;
                }
            } else {
                fputs(line, fp);
            }
        }
        fclose(in);
    }

    if (!replaced) {
        fprintf(fp, "%s,%lu,%lu,%llu\n",
                wanted, totals.sent, totals.received, totals.bytes);
    }

    fclose(fp);
    if (rename(temp_path, path) != 0) {
        perror(path);
        return -1;
    }
    return 0;
}

int read_history_total(const char *root, const char *domain_name, Totals *totals)
{
    char path[ER_MAX_PATH * 2];
    FILE *fp;
    char line[512];

    memset(totals, 0, sizeof(*totals));

    if (history_path(root, domain_name, path, sizeof(path)) != 0) {
        return -1;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        return 0;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        unsigned long sent;
        unsigned long received;
        unsigned long long bytes;
        char date[16];

        if (sscanf(line, "%15[^,],%lu,%lu,%llu", date, &sent, &received, &bytes) == 4) {
            totals->sent += sent;
            totals->received += received;
            totals->bytes += bytes;
        }
    }

    fclose(fp);
    return 0;
}
