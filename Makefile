CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -Iinclude
LDFLAGS ?=

APP := exim4-reporter
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
TEST_SRC := $(wildcard tests/*.c)
TEST_BIN := tests/test_exim4_report
PREFIX ?= /usr/local
SYSCONFDIR ?= /etc
SYSTEMD_DIR ?= /etc/systemd/system
BINDIR := $(PREFIX)/bin
CONFIG_DIR := $(SYSCONFDIR)/exim4-reporter
SERVICE_FILE := packaging/exim4-reporter.service

.PHONY: all clean test install uninstall

all: $(APP)

$(APP): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(TEST_BIN): $(filter-out src/main.o,$(OBJ)) $(TEST_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(filter-out src/main.o,$(OBJ)) $(TEST_SRC)

$(SERVICE_FILE): packaging/exim4-reporter.service.in
	sed -e 's|@BINDIR@|$(BINDIR)|g' -e 's|@CONFIG_DIR@|$(CONFIG_DIR)|g' $< > $@

test: $(TEST_BIN)
	./$(TEST_BIN)

install: $(APP) $(SERVICE_FILE)
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 0755 "$(APP)" "$(DESTDIR)$(BINDIR)/exim4-reporter"
	install -d "$(DESTDIR)$(CONFIG_DIR)"
	install -m 0644 packaging/reporter.conf "$(DESTDIR)$(CONFIG_DIR)/reporter.conf"
	install -m 0644 packaging/domains "$(DESTDIR)$(CONFIG_DIR)/domains"
	install -d "$(DESTDIR)$(SYSTEMD_DIR)"
	install -m 0644 "$(SERVICE_FILE)" "$(DESTDIR)$(SYSTEMD_DIR)/exim4-reporter.service"
	@echo "Installed exim4-reporter."
	@echo "Edit $(CONFIG_DIR)/reporter.conf and $(CONFIG_DIR)/domains."
	@echo "Enable with: systemctl enable --now exim4-reporter.service"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/exim4-reporter"
	rm -f "$(DESTDIR)$(SYSTEMD_DIR)/exim4-reporter.service"

clean:
	rm -f $(APP) exim4-report $(OBJ) $(TEST_BIN) $(SERVICE_FILE)
