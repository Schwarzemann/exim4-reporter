#include "exim4_report.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t keep_running = 1;

static void stop_daemon(int signo)
{
    (void)signo;
    keep_running = 0;
}

int run_report_once(const AppConfig *config, ReportDate date)
{
    Stats stats;
    int rc = -1;

    if (stats_init(&stats) != 0) {
        return -1;
    }
    if (stats_load_domains(&stats, config->domains_file) != 0) {
        goto done;
    }
    if (process_log_file(config->log_file, date, &stats) != 0) {
        goto done;
    }
    if (write_reports(config, &stats, date) != 0) {
        goto done;
    }

    rc = 0;

done:
    stats_free(&stats);
    return rc;
}

static void sleep_interruptible(unsigned int seconds)
{
    while (keep_running && seconds > 0) {
        seconds = sleep(seconds);
    }
}

int run_report_daemon(const char *config_path)
{
    AppConfig config;

    signal(SIGTERM, stop_daemon);
    signal(SIGINT, stop_daemon);

    while (keep_running) {
        ReportDate date = today_local();
        time_t now = time(NULL);

        if (config_load(config_path, &config) != 0) {
            fprintf(stderr, "Unable to load config %s\n", config_path);
        } else if (run_report_once(&config, date) == 0) {
            fprintf(stdout, "Generated report at %ld for %04d-%02d-%02d in %s\n",
                    (long)now, date.year, date.month, date.day, config.output_dir);
            fflush(stdout);
        } else {
            fprintf(stderr, "Report generation failed at %ld\n", (long)now);
        }

        sleep_interruptible(config.report_interval_seconds == 0 ? 3600 : config.report_interval_seconds);
    }

    return 0;
}
