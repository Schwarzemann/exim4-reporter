#include "exim4_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void config_defaults(AppConfig *config)
{
    memset(config, 0, sizeof(*config));
    strcpy(config->log_file, "/var/log/exim4/mainlog");
    strcpy(config->domains_file, "/etc/exim4-reporter/domains");
    strcpy(config->output_dir, "/var/www/html/exim4-reporter");
    strcpy(config->site_name, "Exim4 Mail Report");
    config->top_limit = 100;
    config->report_interval_seconds = 3600;
}

static int copy_value(char *dest, size_t size, const char *value)
{
    if (strlen(value) >= size) {
        return -1;
    }
    strcpy(dest, value);
    return 0;
}

int config_load(const char *path, AppConfig *config)
{
    FILE *fp;
    char line[1024];
    unsigned long top;

    config_defaults(config);

    fp = fopen(path, "r");
    if (fp == NULL) {
        perror(path);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *eq;
        char *key;
        char *value;
        char *comment;

        comment = strchr(line, '#');
        if (comment) {
            *comment = '\0';
        }

        trim(line);
        if (line[0] == '\0') {
            continue;
        }

        eq = strchr(line, '=');
        if (eq == NULL) {
            fprintf(stderr, "Invalid config line: %s\n", line);
            fclose(fp);
            return -1;
        }

        *eq = '\0';
        key = line;
        value = eq + 1;
        trim(key);
        trim(value);

        if (strcmp(key, "log_file") == 0) {
            if (copy_value(config->log_file, sizeof(config->log_file), value) != 0) {
                fclose(fp);
                return -1;
            }
        } else if (strcmp(key, "domains_file") == 0) {
            if (copy_value(config->domains_file, sizeof(config->domains_file), value) != 0) {
                fclose(fp);
                return -1;
            }
        } else if (strcmp(key, "output_dir") == 0) {
            if (copy_value(config->output_dir, sizeof(config->output_dir), value) != 0) {
                fclose(fp);
                return -1;
            }
        } else if (strcmp(key, "site_name") == 0) {
            if (copy_value(config->site_name, sizeof(config->site_name), value) != 0) {
                fclose(fp);
                return -1;
            }
        } else if (strcmp(key, "top_limit") == 0) {
            top = strtoul(value, NULL, 10);
            config->top_limit = top == 0 ? 100 : (size_t)top;
        } else if (strcmp(key, "report_interval_seconds") == 0) {
            top = strtoul(value, NULL, 10);
            config->report_interval_seconds = top == 0 ? 3600 : (unsigned int)top;
        } else {
            fprintf(stderr, "Unknown config key: %s\n", key);
        }
    }

    fclose(fp);
    return 0;
}
