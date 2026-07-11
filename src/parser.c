#include "exim4_report.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_address(const char *cursor, char *email, size_t email_size)
{
    size_t n = 0;

    while (*cursor && isspace((unsigned char)*cursor)) {
        cursor++;
    }
    if (*cursor == '<') {
        cursor++;
    }
    if (*cursor == '>' || *cursor == '\0') {
        return -1;
    }

    while (*cursor && !isspace((unsigned char)*cursor) && *cursor != '>' && n + 1 < email_size) {
        email[n++] = *cursor++;
    }
    email[n] = '\0';

    if (strchr(email, '@') == NULL) {
        return -1;
    }

    lowercase_ascii(email);
    return 0;
}

static int domain_from_email(const char *email, char *domain, size_t domain_size)
{
    const char *at = strrchr(email, '@');

    if (at == NULL || at[1] == '\0' || strlen(at + 1) >= domain_size) {
        return -1;
    }

    strcpy(domain, at + 1);
    lowercase_ascii(domain);
    return 0;
}

static unsigned long long parse_size(const char *line)
{
    const char *p = strstr(line, " S=");

    if (p == NULL) {
        return 0;
    }

    p += 3;
    return strtoull(p, NULL, 10);
}

int parse_exim_line(const char *line, ReportDate date, LogEvent *event)
{
    const char *marker;

    memset(event, 0, sizeof(*event));
    event->type = LOG_EVENT_NONE;

    if (!same_report_date(line, date)) {
        return 0;
    }

    marker = strstr(line, " <= ");
    if (marker != NULL) {
        if (read_address(marker + 4, event->email, sizeof(event->email)) != 0) {
            return 0;
        }
        if (domain_from_email(event->email, event->domain, sizeof(event->domain)) != 0) {
            return 0;
        }
        event->type = LOG_EVENT_SENT;
        event->bytes = parse_size(line);
        return 1;
    }

    marker = strstr(line, " => ");
    if (marker != NULL) {
        if (read_address(marker + 4, event->email, sizeof(event->email)) != 0) {
            return 0;
        }
        if (domain_from_email(event->email, event->domain, sizeof(event->domain)) != 0) {
            return 0;
        }
        event->type = LOG_EVENT_RECEIVED;
        return 1;
    }

    return 0;
}

int process_log_file(const char *path, ReportDate date, Stats *stats)
{
    FILE *fp = fopen(path, "r");
    char line[4096];

    if (fp == NULL) {
        perror(path);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        LogEvent event;
        if (parse_exim_line(line, date, &event) > 0) {
            stats_apply_event(stats, &event);
        }
    }

    fclose(fp);
    return 0;
}
