#ifndef EXIM4_REPORT_H
#define EXIM4_REPORT_H

#include <stddef.h>
#include <time.h>

#define ER_MAX_PATH 512
#define ER_MAX_NAME 128
#define ER_MAX_EMAIL 320
#define ER_DEFAULT_CONFIG "/etc/exim4-reporter/reporter.conf"

typedef struct {
    char log_file[ER_MAX_PATH];
    char domains_file[ER_MAX_PATH];
    char output_dir[ER_MAX_PATH];
    char site_name[ER_MAX_NAME];
    size_t top_limit;
    unsigned int report_interval_seconds;
} AppConfig;

typedef struct {
    int year;
    int month;
    int day;
} ReportDate;

typedef struct {
    char email[ER_MAX_EMAIL];
    unsigned long sent;
    unsigned long received;
    unsigned long long bytes;
} UserStats;

typedef struct {
    char name[ER_MAX_NAME];
    unsigned long sent;
    unsigned long received;
    unsigned long long bytes;
    UserStats *users;
    size_t user_count;
    size_t user_cap;
} DomainStats;

typedef struct {
    DomainStats *domains;
    size_t domain_count;
    size_t domain_cap;
    DomainStats general;
} Stats;

typedef struct {
    unsigned long sent;
    unsigned long received;
    unsigned long long bytes;
} Totals;

typedef enum {
    LOG_EVENT_NONE = 0,
    LOG_EVENT_SENT,
    LOG_EVENT_RECEIVED
} LogEventType;

typedef struct {
    LogEventType type;
    char email[ER_MAX_EMAIL];
    char domain[ER_MAX_NAME];
    unsigned long long bytes;
} LogEvent;

int config_load(const char *path, AppConfig *config);
void config_defaults(AppConfig *config);

int parse_date(const char *text, ReportDate *date);
ReportDate today_local(void);
int same_report_date(const char *line, ReportDate date);
int parse_exim_line(const char *line, ReportDate date, LogEvent *event);

int stats_init(Stats *stats);
void stats_free(Stats *stats);
int stats_load_domains(Stats *stats, const char *path);
void stats_apply_event(Stats *stats, const LogEvent *event);

int process_log_file(const char *path, ReportDate date, Stats *stats);
int run_report_once(const AppConfig *config, ReportDate date);
int run_report_daemon(const char *config_path);

int write_reports(const AppConfig *config, const Stats *stats, ReportDate date);
int update_history(const char *root, const char *domain_name, ReportDate date, Totals totals);
int read_history_total(const char *root, const char *domain_name, Totals *totals);

Totals totals_from_domain(const DomainStats *domain);

int ensure_dir(const char *path);
int ensure_parent_dirs(const char *path);
void trim(char *text);
void lowercase_ascii(char *text);
const char *path_basename(const char *path);

#endif
