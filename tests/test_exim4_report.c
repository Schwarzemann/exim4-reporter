#include "exim4_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require_int(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "test failed: %s\n", message);
        exit(1);
    }
}

static void test_parse_sender(void)
{
    ReportDate date;
    LogEvent event;
    const char *line = "2026-06-26 12:00:00 1x <= Alice@Example.COM H=host P=esmtp S=4096";

    require_int(parse_date("2026-06-26", &date) == 0, "parse date");
    require_int(parse_exim_line(line, date, &event) == 1, "parse sender line");
    require_int(event.type == LOG_EVENT_SENT, "sender event type");
    require_int(strcmp(event.email, "alice@example.com") == 0, "sender email lowercase");
    require_int(strcmp(event.domain, "example.com") == 0, "sender domain");
    require_int(event.bytes == 4096, "sender bytes");
}

static void test_parse_recipient(void)
{
    ReportDate date;
    LogEvent event;
    const char *line = "2026-06-26 12:00:01 1x => Bob@Example.NET R=dnslookup T=remote_smtp";

    require_int(parse_date("2026-06-26", &date) == 0, "parse date");
    require_int(parse_exim_line(line, date, &event) == 1, "parse recipient line");
    require_int(event.type == LOG_EVENT_RECEIVED, "recipient event type");
    require_int(strcmp(event.email, "bob@example.net") == 0, "recipient email lowercase");
    require_int(strcmp(event.domain, "example.net") == 0, "recipient domain");
}

static void test_skip_other_day(void)
{
    ReportDate date;
    LogEvent event;
    const char *line = "2026-06-25 12:00:00 1x <= alice@example.com H=host S=1";

    require_int(parse_date("2026-06-26", &date) == 0, "parse date");
    require_int(parse_exim_line(line, date, &event) == 0, "skip different day");
}

static void test_stats(void)
{
    Stats stats;
    LogEvent sent;
    LogEvent received;

    require_int(stats_init(&stats) == 0, "stats init");

    FILE *fp = fopen("tests/domains.tmp", "w");
    require_int(fp != NULL, "create temp domains");
    fputs("example.com\nexample.net\n", fp);
    fclose(fp);

    require_int(stats_load_domains(&stats, "tests/domains.tmp") == 0, "load temp domains");

    memset(&sent, 0, sizeof(sent));
    sent.type = LOG_EVENT_SENT;
    strcpy(sent.email, "alice@example.com");
    strcpy(sent.domain, "example.com");
    sent.bytes = 99;
    stats_apply_event(&stats, &sent);

    memset(&received, 0, sizeof(received));
    received.type = LOG_EVENT_RECEIVED;
    strcpy(received.email, "bob@example.net");
    strcpy(received.domain, "example.net");
    stats_apply_event(&stats, &received);

    require_int(stats.domains[0].sent == 1, "domain sent count");
    require_int(stats.domains[0].bytes == 99, "domain bytes");
    require_int(stats.domains[1].received == 1, "domain received count");
    require_int(stats.general.sent == 1, "general sent count");
    require_int(stats.general.received == 1, "general received count");

    remove("tests/domains.tmp");
    stats_free(&stats);
}

int main(void)
{
    test_parse_sender();
    test_parse_recipient();
    test_skip_other_day();
    test_stats();
    puts("all tests passed");
    return 0;
}
