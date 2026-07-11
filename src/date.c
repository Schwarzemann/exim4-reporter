#include "exim4_report.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

int parse_date(const char *text, ReportDate *date)
{
    int y;
    int m;
    int d;
    char tail;

    if (sscanf(text, "%d-%d-%d%c", &y, &m, &d, &tail) != 3) {
        return -1;
    }
    if (y < 1970 || m < 1 || m > 12 || d < 1 || d > 31) {
        return -1;
    }

    date->year = y;
    date->month = m;
    date->day = d;
    return 0;
}

ReportDate today_local(void)
{
    time_t now = time(NULL);
    struct tm tmv;
    ReportDate date;

    tmv = *localtime(&now);
    date.year = tmv.tm_year + 1900;
    date.month = tmv.tm_mon + 1;
    date.day = tmv.tm_mday;
    return date;
}

int same_report_date(const char *line, ReportDate date)
{
    char prefix[11];

    snprintf(prefix, sizeof(prefix), "%04d-%02d-%02d", date.year, date.month, date.day);
    return strncmp(line, prefix, 10) == 0;
}
