#include "exim4_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int init_domain(DomainStats *domain, const char *name)
{
    memset(domain, 0, sizeof(*domain));
    if (strlen(name) >= sizeof(domain->name)) {
        return -1;
    }
    strcpy(domain->name, name);
    lowercase_ascii(domain->name);
    return 0;
}

int stats_init(Stats *stats)
{
    memset(stats, 0, sizeof(*stats));
    return init_domain(&stats->general, "general");
}

static void free_domain(DomainStats *domain)
{
    free(domain->users);
    domain->users = NULL;
    domain->user_count = 0;
    domain->user_cap = 0;
}

void stats_free(Stats *stats)
{
    for (size_t i = 0; i < stats->domain_count; i++) {
        free_domain(&stats->domains[i]);
    }
    free(stats->domains);
    free_domain(&stats->general);
    memset(stats, 0, sizeof(*stats));
}

static DomainStats *find_domain(Stats *stats, const char *name)
{
    for (size_t i = 0; i < stats->domain_count; i++) {
        if (strcmp(stats->domains[i].name, name) == 0) {
            return &stats->domains[i];
        }
    }
    return NULL;
}

static int add_domain(Stats *stats, const char *name)
{
    DomainStats *new_domains;
    char normalized[ER_MAX_NAME];

    if (strlen(name) >= sizeof(normalized)) {
        return -1;
    }
    strcpy(normalized, name);
    lowercase_ascii(normalized);

    if (find_domain(stats, normalized) != NULL) {
        return 0;
    }

    if (stats->domain_count == stats->domain_cap) {
        size_t next_cap = stats->domain_cap == 0 ? 8 : stats->domain_cap * 2;
        new_domains = realloc(stats->domains, next_cap * sizeof(*stats->domains));
        if (new_domains == NULL) {
            return -1;
        }
        stats->domains = new_domains;
        stats->domain_cap = next_cap;
    }

    if (init_domain(&stats->domains[stats->domain_count], normalized) != 0) {
        return -1;
    }
    stats->domain_count++;
    return 0;
}

int stats_load_domains(Stats *stats, const char *path)
{
    FILE *fp = fopen(path, "r");
    char line[512];
    size_t loaded = 0;

    if (fp == NULL) {
        perror(path);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *comment = strchr(line, '#');
        if (comment) {
            *comment = '\0';
        }
        trim(line);
        lowercase_ascii(line);
        if (line[0] == '\0') {
            continue;
        }
        if (add_domain(stats, line) != 0) {
            fclose(fp);
            return -1;
        }
        loaded++;
    }

    fclose(fp);
    if (loaded == 0) {
        fprintf(stderr, "No domains loaded from %s\n", path);
        return -1;
    }
    return 0;
}

static UserStats *find_or_add_user(DomainStats *domain, const char *email)
{
    UserStats *new_users;

    for (size_t i = 0; i < domain->user_count; i++) {
        if (strcmp(domain->users[i].email, email) == 0) {
            return &domain->users[i];
        }
    }

    if (domain->user_count == domain->user_cap) {
        size_t next_cap = domain->user_cap == 0 ? 16 : domain->user_cap * 2;
        new_users = realloc(domain->users, next_cap * sizeof(*domain->users));
        if (new_users == NULL) {
            return NULL;
        }
        domain->users = new_users;
        domain->user_cap = next_cap;
    }

    memset(&domain->users[domain->user_count], 0, sizeof(domain->users[domain->user_count]));
    if (strlen(email) >= sizeof(domain->users[domain->user_count].email)) {
        return NULL;
    }
    strcpy(domain->users[domain->user_count].email, email);
    domain->user_count++;
    return &domain->users[domain->user_count - 1];
}

static void apply_to_domain(DomainStats *domain, const LogEvent *event)
{
    UserStats *user = find_or_add_user(domain, event->email);
    if (user == NULL) {
        return;
    }

    if (event->type == LOG_EVENT_SENT) {
        domain->sent++;
        domain->bytes += event->bytes;
        user->sent++;
        user->bytes += event->bytes;
    } else if (event->type == LOG_EVENT_RECEIVED) {
        domain->received++;
        user->received++;
    }
}

void stats_apply_event(Stats *stats, const LogEvent *event)
{
    DomainStats *domain;

    if (event->type == LOG_EVENT_NONE) {
        return;
    }

    domain = find_domain(stats, event->domain);
    if (domain != NULL) {
        apply_to_domain(domain, event);
        apply_to_domain(&stats->general, event);
    }
}

Totals totals_from_domain(const DomainStats *domain)
{
    Totals totals;
    totals.sent = domain->sent;
    totals.received = domain->received;
    totals.bytes = domain->bytes;
    return totals;
}
