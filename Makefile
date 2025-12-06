CC := gcc
CFLAGS := -std=c99 -g -pedantic-errors -Werror -Wall -Wextra -Wpedantic \
	-Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-security -Wnull-dereference \
	-Wstack-protector -Wtrampolines -Walloca -Wvla -Warray-bounds=2 \
	-Wimplicit-fallthrough=3 -Wshift-overflow=2 -Wcast-qual -Wstringop-overflow=4 \
	-Wconversion -Wsign-conversion -Wlogical-op -Wduplicated-cond -Wduplicated-branches \
	-Wrestrict -Wshadow -Wundef -Wredundant-decls -Wfloat-equal -Wbad-function-cast \
	-Wcast-align -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
	-Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn -Wmissing-format-attribute \
	-Wpacked -Wpadded -Waggregate-return -Wswitch-default -Wswitch-enum -Wunreachable-code \
	-Winline -Winvalid-pch -Wdisabled-optimization -Wdouble-promotion -Wunsafe-loop-optimizations \
	-Wvector-operation-performance -Wunused -Wunused-macros -Wunused-const-variable \
	-Wunused-parameter -Wc++-compat -Iinclude -Isrc/settings -MMD -MP

LDFLAGS := -pthread
LDLIBS_COMMON := -lncurses -ltinfo
LDLIBS_GUI := $(LDLIBS_COMMON) -lX11 -lXrandr -lm -lwayland-client

PREFIX ?= $(HOME)/.local
BINDIR := $(PREFIX)/bin
LIBDIR := $(PREFIX)/lib
DATADIR := $(PREFIX)/share
APPSDIR := $(DATADIR)/applications
INSTALL_LIB := $(LIBDIR)/gammatui

BUILD_DIR := build
TARGETS := $(BUILD_DIR)/menu/menu.elf $(BUILD_DIR)/gammatui/gammatui.elf $(BUILD_DIR)/settings/brightnesstui.elf

SRCS_GAMMATUI := src/gammatui/main.c src/gammatui/ui.c src/gammatui/xrandr.c src/gammatui/gamma_control.c src/gammatui/wlr-gamma-control-unstable-v1-protocol.c src/settings/config.c
SRCS_MENU := src/menu/menu.c src/menu/proc.c src/menu/ui.c src/menu/utils.c src/settings/config.c
SRCS_SETTINGS := src/settings/brightnesstui.c src/settings/config.c

OBJS_GAMMATUI := $(SRCS_GAMMATUI:src/%.c=$(BUILD_DIR)/%.o)
OBJS_MENU := $(SRCS_MENU:src/%.c=$(BUILD_DIR)/%.o)
OBJS_SETTINGS := $(SRCS_SETTINGS:src/%.c=$(BUILD_DIR)/%.o)
ALL_OBJS := $(OBJS_GAMMATUI) $(OBJS_MENU) $(OBJS_SETTINGS)
DEPS := $(ALL_OBJS:.o=.d)

define DESKTOP_ENTRY
[Desktop Entry]
Type=Application
Name=gammatui
Comment=A lightweight, highly customizable text user interface to adjust gamma, brightness, and related effects for an output.
Exec=$(BINDIR)/gammatui
Terminal=true
Categories=Utility;Settings;
Icon=utilities-terminal
endef
export DESKTOP_ENTRY

all: $(TARGETS)

$(BUILD_DIR)/gammatui/gammatui.elf: LDLIBS := $(LDLIBS_GUI)
$(BUILD_DIR)/gammatui/gammatui.elf: $(OBJS_GAMMATUI)

$(BUILD_DIR)/menu/menu.elf: LDLIBS := $(LDLIBS_COMMON)
$(BUILD_DIR)/menu/menu.elf: $(OBJS_MENU)

$(BUILD_DIR)/settings/brightnesstui.elf: LDLIBS := $(LDLIBS_COMMON)
$(BUILD_DIR)/settings/brightnesstui.elf: $(OBJS_SETTINGS)

$(TARGETS):
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

run: all
	@$(BUILD_DIR)/menu/menu.elf

install: all
	install -D -m 755 $(BUILD_DIR)/menu/menu.elf $(DESTDIR)$(INSTALL_LIB)/menu/menu.elf
	install -D -m 755 $(BUILD_DIR)/gammatui/gammatui.elf $(DESTDIR)$(INSTALL_LIB)/gammatui/gammatui.elf
	install -D -m 755 $(BUILD_DIR)/settings/brightnesstui.elf $(DESTDIR)$(INSTALL_LIB)/settings/brightnesstui.elf
	install -D -m 644 src/settings/config.json $(DESTDIR)$(INSTALL_LIB)/settings/config.json
	install -d $(DESTDIR)$(BINDIR)
	ln -sf $(INSTALL_LIB)/menu/menu.elf $(DESTDIR)$(BINDIR)/gammatui
	install -d $(DESTDIR)$(APPSDIR)
	@echo "$$DESKTOP_ENTRY" > $(DESTDIR)$(APPSDIR)/gammatui.desktop

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/gammatui
	rm -rf $(DESTDIR)$(INSTALL_LIB)
	rm -f $(DESTDIR)$(APPSDIR)/gammatui.desktop

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run install uninstall
