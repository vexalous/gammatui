CC = gcc
CFLAGS = -std=c99 -pedantic-errors -Werror -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wconversion -Wunreachable-code -Iinclude -g
LDFLAGS = -pthread
LDLIBS_GAMMATUI = -lncurses -ltinfo -lX11 -lXrandr -lm
LDLIBS_MENU = -lncurses -ltinfo
LDLIBS_SETTINGS = -lncurses -ltinfo

BUILD_DIR = build
MENU_DIR = $(BUILD_DIR)/menu
GAMMATUI_DIR = $(BUILD_DIR)/gammatui
SETTINGS_DIR = $(BUILD_DIR)/settings

TARGET_MENU = $(MENU_DIR)/menu.elf
TARGET_GAMMATUI = $(GAMMATUI_DIR)/gammatui.elf
TARGET_SETTINGS = $(SETTINGS_DIR)/brightnesstui.elf

GAMMATUI_SRCS = src/gammatui/main.c src/gammatui/ui.c src/gammatui/xrandr.c src/gammatui/gamma_control.c
MENU_SRCS = src/menu/menu.c src/menu/proc.c src/menu/ui.c src/menu/utils.c
SETTINGS_SRCS = src/settings/brightnesstui.c src/settings/config.c

GAMMATUI_OBJS = $(patsubst src/gammatui/%.c, $(GAMMATUI_DIR)/%.o, $(GAMMATUI_SRCS))
MENU_OBJS = $(patsubst src/menu/%.c, $(MENU_DIR)/%.o, $(MENU_SRCS))
SETTINGS_OBJS = $(patsubst src/settings/%.c, $(SETTINGS_DIR)/%.o, $(SETTINGS_SRCS))

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

$(GAMMATUI_DIR)/%.o: src/gammatui/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled $<"

$(MENU_DIR)/%.o: src/menu/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled $<"

$(SETTINGS_DIR)/%.o: src/settings/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled $<"

run: all
	@echo "Starting application..."
	@$(TARGET_MENU)

clean:
	@echo "Cleaning up build files..."
	@rm -rf $(BUILD_DIR)

.PHONY: all clean run
