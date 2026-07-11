# exim4-reporter

`exim4-reporter` is a simple Exim4 log analyzer. It reads an Exim4 mainlog, counts mail activity for configured domains, stores lightweight CSV history, and writes Markdown reports that are easy to read.

## Build

```sh
make
```

## Run

```sh
./exim4-reporter -c /etc/exim4-reporter/reporter.conf
```

Use a specific report date:

```sh
./exim4-reporter -c /etc/exim4-reporter/reporter.conf -d 2026-06-26
```

Without `-c`, the program reads:

```sh
/etc/exim4-reporter/reporter.conf
```

## Install

```sh
sudo make install
sudo systemctl enable --now exim4-reporter.service
```

`make install` installs:

- `/usr/local/bin/exim4-reporter`
- `/etc/exim4-reporter/reporter.conf`
- `/etc/exim4-reporter/domains`
- `/etc/systemd/system/exim4-reporter.service`

For package builds or unprivileged testing:

```sh
make install DESTDIR=/tmp/exim4-reporter-root
```

## Daemon Mode

```sh
exim4-reporter --daemon
```

## Config

```conf
log_file = /var/log/exim4/mainlog
domains_file = /etc/exim4-reporter/domains
output_dir = /var/www/html/exim4-reporter
site_name = Mail Report
top_limit = 100
report_interval_seconds = 3600
```

## Reports

The generated output contains:

- `index.md`: domain list and global link
- `DOMAIN/YYYY/MM/DD.md`: daily domain report
- `DOMAIN/YYYY/MM/index.md`: monthly domain summary
- `DOMAIN/YYYY/index.md`: yearly domain summary
- `DOMAIN/index.md`: all-years domain summary
- matching pages under `general/` for all configured domains combined
