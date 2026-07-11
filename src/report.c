#include "exim4_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*UserCmp)(const void *, const void *);

static void md_escape(FILE *fp, const char *text)
{
    for (; *text; text++) {
        switch (*text) {
        case '\\':
        case '`':
        case '*':
        case '_':
        case '[':
        case ']':
        case '<':
        case '>':
        case '|':
            fputc('\\', fp);
            fputc(*text, fp);
            break;
        default:
            fputc(*text, fp);
            break;
        }
    }
}

static void size_text(unsigned long long bytes, char *buf, size_t size)
{
    const char *unit = "bytes";
    double value = (double)bytes;

    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        value = value / (1024.0 * 1024.0 * 1024.0);
        unit = "GB";
    } else if (bytes >= 1024ULL * 1024ULL) {
        value = value / (1024.0 * 1024.0);
        unit = "MB";
    } else if (bytes >= 1024ULL) {
        value = value / 1024.0;
        unit = "KB";
    }

    snprintf(buf, size, "%.2f %s", value, unit);
}

static int cmp_sent(const void *a, const void *b)
{
    const UserStats *ua = *(const UserStats * const *)a;
    const UserStats *ub = *(const UserStats * const *)b;
    return (ub->sent > ua->sent) - (ub->sent < ua->sent);
}

static int cmp_received(const void *a, const void *b)
{
    const UserStats *ua = *(const UserStats * const *)a;
    const UserStats *ub = *(const UserStats * const *)b;
    return (ub->received > ua->received) - (ub->received < ua->received);
}

static int cmp_total(const void *a, const void *b)
{
    const UserStats *ua = *(const UserStats * const *)a;
    const UserStats *ub = *(const UserStats * const *)b;
    unsigned long ta = ua->sent + ua->received;
    unsigned long tb = ub->sent + ub->received;
    return (tb > ta) - (tb < ta);
}

static int cmp_bytes(const void *a, const void *b)
{
    const UserStats *ua = *(const UserStats * const *)a;
    const UserStats *ub = *(const UserStats * const *)b;
    return (ub->bytes > ua->bytes) - (ub->bytes < ua->bytes);
}

static void page_begin(FILE *fp, const AppConfig *config, const char *title)
{
    fprintf(fp, "# ");
    md_escape(fp, config->site_name);
    fprintf(fp, "\n\n## ");
    md_escape(fp, title);
    fprintf(fp, "\n\n");
}

static void summary_table(FILE *fp, const char *sent_label, const char *received_label,
                          const char *bytes_label, unsigned long sent,
                          unsigned long received, unsigned long long total_bytes)
{
    char bytes[64];
    size_text(total_bytes, bytes, sizeof(bytes));
    fprintf(fp, "| %s | %s | %s |\n", sent_label, received_label, bytes_label);
    fprintf(fp, "| --- | --- | --- |\n");
    fprintf(fp, "| %lu | %lu | %s |\n\n", sent, received, bytes);
}

static void users_table(FILE *fp, const DomainStats *domain, size_t limit, const char *title, UserCmp cmp)
{
    UserStats **rows;
    size_t row_count = domain->user_count < limit ? domain->user_count : limit;

    rows = malloc(domain->user_count * sizeof(*rows));
    if (rows == NULL) {
        return;
    }
    for (size_t i = 0; i < domain->user_count; i++) {
        rows[i] = &domain->users[i];
    }
    qsort(rows, domain->user_count, sizeof(*rows), cmp);

    fprintf(fp, "### %s\n\n", title);
    fprintf(fp, "| # | User | Sent | Received | Total | Bytes |\n");
    fprintf(fp, "| --- | --- | --- | --- | --- | --- |\n");
    for (size_t i = 0; i < row_count; i++) {
        char bytes[64];
        const UserStats *u = rows[i];
        if (u->sent + u->received == 0) {
            continue;
        }
        size_text(u->bytes, bytes, sizeof(bytes));
        fprintf(fp, "| %zu | ", i + 1);
        md_escape(fp, u->email);
        fprintf(fp, " | %lu | %lu | %lu | %s |\n",
                u->sent, u->received, u->sent + u->received, bytes);
    }
    fprintf(fp, "\n");
    free(rows);
}

static int write_daily(const AppConfig *config, const DomainStats *domain, ReportDate date)
{
    char path[ER_MAX_PATH * 2];
    char title[256];
    FILE *fp;

    snprintf(path, sizeof(path), "%s/%s/%04d/%02d/%02d.md",
             config->output_dir, domain->name, date.year, date.month, date.day);
    if (ensure_parent_dirs(path) != 0) {
        return -1;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        perror(path);
        return -1;
    }

    snprintf(title, sizeof(title), "%s daily report for %04d-%02d-%02d",
             domain->name, date.year, date.month, date.day);
    page_begin(fp, config, title);
    summary_table(fp, "Sent", "Received", "Sent size",
                  domain->sent, domain->received, domain->bytes);
    users_table(fp, domain, config->top_limit, "Top senders", cmp_sent);
    users_table(fp, domain, config->top_limit, "Top receivers", cmp_received);
    users_table(fp, domain, config->top_limit, "Top total", cmp_total);
    users_table(fp, domain, config->top_limit, "Top by sent size", cmp_bytes);
    fclose(fp);
    return 0;
}

static int write_history_page(const AppConfig *config, const char *domain_name, const char *path, const char *title)
{
    FILE *fp;
    Totals totals;

    if (ensure_parent_dirs(path) != 0) {
        return -1;
    }
    if (read_history_total(config->output_dir, domain_name, &totals) != 0) {
        return -1;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        perror(path);
        return -1;
    }
    page_begin(fp, config, title);
    summary_table(fp, "Historical sent", "Historical received", "Historical sent size",
                  totals.sent, totals.received, totals.bytes);
    fclose(fp);
    return 0;
}

static int write_domain_pages(const AppConfig *config, const DomainStats *domain, ReportDate date)
{
    char path[ER_MAX_PATH * 2];
    char title[256];

    if (write_daily(config, domain, date) != 0) {
        return -1;
    }
    if (update_history(config->output_dir, domain->name, date, totals_from_domain(domain)) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s/%04d/%02d/index.md",
             config->output_dir, domain->name, date.year, date.month);
    snprintf(title, sizeof(title), "%s monthly history", domain->name);
    if (write_history_page(config, domain->name, path, title) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s/%04d/index.md",
             config->output_dir, domain->name, date.year);
    snprintf(title, sizeof(title), "%s yearly history", domain->name);
    if (write_history_page(config, domain->name, path, title) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s/index.md", config->output_dir, domain->name);
    snprintf(title, sizeof(title), "%s all history", domain->name);
    return write_history_page(config, domain->name, path, title);
}

static int write_index(const AppConfig *config, const Stats *stats, ReportDate date)
{
    char path[ER_MAX_PATH * 2];
    FILE *fp;

    snprintf(path, sizeof(path), "%s/index.md", config->output_dir);
    if (ensure_parent_dirs(path) != 0) {
        return -1;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        perror(path);
        return -1;
    }
    page_begin(fp, config, "Index");
    fprintf(fp, "| Domain | Daily report | Sent | Received | Total |\n");
    fprintf(fp, "| --- | --- | --- | --- | --- |\n");
    for (size_t i = 0; i < stats->domain_count; i++) {
        const DomainStats *d = &stats->domains[i];
        fprintf(fp, "| ");
        md_escape(fp, d->name);
        fprintf(fp, " | [open](%s/%04d/%02d/%02d.md) | %lu | %lu | %lu |\n",
                d->name, date.year, date.month, date.day, d->sent, d->received, d->sent + d->received);
    }
    fprintf(fp, "| general | [open](general/%04d/%02d/%02d.md) | %lu | %lu | %lu |\n",
            date.year, date.month, date.day,
            stats->general.sent, stats->general.received, stats->general.sent + stats->general.received);
    fclose(fp);
    return 0;
}

int write_reports(const AppConfig *config, const Stats *stats, ReportDate date)
{
    if (ensure_dir(config->output_dir) != 0) {
        return -1;
    }

    for (size_t i = 0; i < stats->domain_count; i++) {
        if (write_domain_pages(config, &stats->domains[i], date) != 0) {
            return -1;
        }
    }

    if (write_domain_pages(config, &stats->general, date) != 0) {
        return -1;
    }

    return write_index(config, stats, date);
}
