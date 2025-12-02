CC = gcc
CFLAGS = -std=c99 -pedantic-errors -Werror -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wconversion -Wunreachable-code -Iinclude -Isrc/settings -g
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
	@echo "Linked ==> $@"

$(TARGET_MENU): $(MENU_OBJS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS_MENU)
	@echo "Linked ==> $@"

$(TARGET_SETTINGS): $(SETTINGS_OBJS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS_SETTINGS)
	@echo "Linked ==> $@"

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled $<"

run: all
	@echo "Starting application..."
	@$(TARGET_MENU)

install: all
	@echo "Installing libraries to $(DESTDIR)$(INSTALL_DIR)..."
	@mkdir -p $(DESTDIR)$(INSTALL_DIR)/menu
	@mkdir -p $(DESTDIR)$(INSTALL_DIR)/gammatui
	@mkdir -p $(DESTDIR)$(INSTALL_DIR)/settings
	install -m 755 $(TARGET_MENU) $(DESTDIR)$(INSTALL_DIR)/menu/menu.elf
	install -m 755 $(TARGET_GAMMATUI) $(DESTDIR)$(INSTALL_DIR)/gammatui/gammatui.elf
	install -m 755 $(TARGET_SETTINGS) $(DESTDIR)$(INSTALL_DIR)/settings/brightnesstui.elf
	install -m 644 src/settings/config.json $(DESTDIR)$(INSTALL_DIR)/settings/config.json
	
	@echo "Installing binary symlink to $(DESTDIR)$(BINDIR)..."
	@mkdir -p $(DESTDIR)$(BINDIR)
	@ln -sf $(INSTALL_DIR)/menu/menu.elf $(DESTDIR)$(BINDIR)/gammatui
	
	@echo "Installing desktop entry to $(DESTDIR)$(APPSDIR)..."
	@mkdir -p $(DESTDIR)$(APPSDIR)
	@echo "[Desktop Entry]" > $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Type=Application" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Name=gammatui" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Comment=A tui to adjust effects such as gamma and brightness for an output. Written in c." >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Exec=$(BINDIR)/gammatui" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Terminal=true" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Categories=Utility;Settings;" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Icon=utilities-terminal" >> $(DESTDIR)$(APPSDIR)/gammatui.desktop

	@echo "Installation complete."

uninstall:
	@echo "Uninstalling from $(DESTDIR)$(INSTALL_DIR)..."
	@rm -f $(DESTDIR)$(BINDIR)/gammatui
	@rm -rf $(DESTDIR)$(INSTALL_DIR)
	@rm -f $(DESTDIR)$(APPSDIR)/gammatui.desktop
	@echo "Uninstallation complete."

clean:
	@echo "Cleaning up build files..."
	@rm -rf $(BUILD_DIR)

.PHONY: all clean run install uninstall
