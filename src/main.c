#include "exim4_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *name)
{
    fprintf(stderr,
            "Usage: %s [-c config] [-d YYYY-MM-DD] [--daemon]\n"
            "  -c  config file, defaults to " ER_DEFAULT_CONFIG "\n"
            "  -d  report date, defaults to local today\n"
            "      ignored in daemon mode\n"
            "  --daemon  run continuously for systemd\n"
            "  -h  show help\n",
            path_basename(name));
}

int main(int argc, char **argv)
{
    const char *config_path = ER_DEFAULT_CONFIG;
    ReportDate date = today_local();
    AppConfig config;
    int rc = 1;
    int daemon_mode = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            config_path = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0) {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            if (parse_date(argv[++i], &date) != 0) {
                fprintf(stderr, "Invalid date: %s\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--daemon") == 0) {
            daemon_mode = 1;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (daemon_mode) {
        return run_report_daemon(config_path);
    }

    if (config_load(config_path, &config) != 0) {
        return 1;
    }
    if (run_report_once(&config, date) != 0) {
        return 1;
    }

    printf("Wrote Exim4 report to %s for %04d-%02d-%02d\n",
           config.output_dir, date.year, date.month, date.day);
    rc = 0;

    return rc;
}
