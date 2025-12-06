CC = gcc
CFLAGS = -std=c99 -g -pedantic-errors -Werror -Wall -Wextra -Wpedantic \
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
	-Wunused-parameter -Wc++-compat -Iinclude -Isrc/settings

LDFLAGS = -pthread
LDLIBS_GAMMATUI = -lncurses -ltinfo -lX11 -lXrandr -lm -lwayland-client
LDLIBS_MENU = -lncurses -ltinfo
LDLIBS_SETTINGS = -lncurses -ltinfo

BUILD_DIR = build
MENU_DIR = $(BUILD_DIR)/menu
GAMMATUI_DIR = $(BUILD_DIR)/gammatui
SETTINGS_DIR = $(BUILD_DIR)/settings

TARGET_MENU = $(MENU_DIR)/menu.elf
TARGET_GAMMATUI = $(GAMMATUI_DIR)/gammatui.elf
TARGET_SETTINGS = $(SETTINGS_DIR)/brightnesstui.elf

GAMMATUI_SRCS = src/gammatui/main.c src/gammatui/ui.c src/gammatui/xrandr.c src/gammatui/gamma_control.c src/gammatui/wlr-gamma-control-unstable-v1-protocol.c src/settings/config.c
MENU_SRCS = src/menu/menu.c src/menu/proc.c src/menu/ui.c src/menu/utils.c src/settings/config.c
SETTINGS_SRCS = src/settings/brightnesstui.c src/settings/config.c

GAMMATUI_OBJS = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(GAMMATUI_SRCS))
MENU_OBJS = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(MENU_SRCS))
SETTINGS_OBJS = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SETTINGS_SRCS))

PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
DATADIR ?= $(PREFIX)/share
APPSDIR ?= $(DATADIR)/applications
INSTALL_DIR = $(LIBDIR)/gammatui

all: $(TARGET_MENU) $(TARGET_GAMMATUI) $(TARGET_SETTINGS)

$(TARGET_GAMMATUI): $(GAMMATUI_OBJS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS_GAMMATUI)

$(TARGET_MENU): $(MENU_OBJS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS_MENU)

$(TARGET_SETTINGS): $(SETTINGS_OBJS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS_SETTINGS)

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	@$(TARGET_MENU)

install: all
	@mkdir -p $(DESTDIR)$(INSTALL_DIR)/menu
	@mkdir -p $(DESTDIR)$(INSTALL_DIR)/gammatui
	@mkdir -p $(DESTDIR)$(INSTALL_DIR)/settings
	install -m 755 $(TARGET_MENU) $(DESTDIR)$(INSTALL_DIR)/menu/menu.elf
	install -m 755 $(TARGET_GAMMATUI) $(DESTDIR)$(INSTALL_DIR)/gammatui/gammatui.elf
	install -m 755 $(TARGET_SETTINGS) $(DESTDIR)$(INSTALL_DIR)/settings/brightnesstui.elf
	install -m 644 src/settings/config.json $(DESTDIR)$(INSTALL_DIR)/settings/config.json
	@mkdir -p $(DESTDIR)$(BINDIR)
	@ln -sf $(INSTALL_DIR)/menu/menu.elf $(DESTDIR)$(BINDIR)/gammatui
	@mkdir -p $(DESTDIR)$(APPSDIR)
	@echo "[Desktop Entry]" > $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Type=Application" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Name=gammatui" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Comment=A lightweight, highly customizable text user interface to adjust gamma, brightness, and related effects for an output." >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Exec=$(BINDIR)/gammatui" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Terminal=true" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Categories=Utility;Settings;" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Icon=utilities-terminal" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop

uninstall:
	@rm -f $(DESTDIR)$(BINDIR)/gammatui
	@rm -rf $(DESTDIR)$(INSTALL_DIR)
	@rm -f $(DESTDIR)$(APPSDIR)/gammatui.desktop

clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all clean run install uninstall
