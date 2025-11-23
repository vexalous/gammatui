#define _POSIX_C_SOURCE 200809L
#include "settings.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stddef.h>

static const char *TITLE = "Gammatui - Settings";
static const char *UI_FIELDS[] = {
    "Output",
    "Gamma Min",
    "Gamma Max",
    "Bright Min",
    "Bright Max",
    "Sel Color",
    "Unsel Color",
    "Save Config"
};

#define UI_START_ROW 5
#define UI_LABEL_COL 4
#define UI_VALUE_COL 20
#define UI_ROW_STEP 2

static const char *COLOR_NAMES[] = {
    "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White"
};

enum {
    FI_OUTPUT = 0,
    FI_GAMMA_MIN,
    FI_GAMMA_MAX,
    FI_BRIGHT_MIN,
    FI_BRIGHT_MAX,
    FI_SEL_COLOR,
    FI_UNSEL_COLOR,
    FI_SAVE,
    FI_COUNT
};

static void show_message(WINDOW *w, const char *title, const char *msg) {
    werase(w);
    box(w, 0, 0);
    mvwprintw(w, 1, 2, "%s", title);
    mvwprintw(w, 3, 2, "%s", msg);
    mvwprintw(w, 5, 2, "Press any key to return");
    wrefresh(w);
    wgetch(w);
}

static void string_insert_char(char *str, int pos, int max_len, char ch) {
    int len = (int)strlen(str);
    if (len >= max_len - 1) return;
    for (int i = len; i > pos; i--) {
        str[i] = str[i-1];
    }
    str[pos] = ch;
    str[len + 1] = '\0';
}

static void string_delete_char(char *str, int pos) {
    int len = (int)strlen(str);
    if (pos >= len) return;
    for (int i = pos; i < len; i++) {
        str[i] = str[i+1];
    }
}

static void edit_string(WINDOW *w, int row, int col, char *buf, size_t bufsz) {
    char scratch[OUTPUT_LEN + 1];
    if (bufsz > sizeof(scratch)) bufsz = sizeof(scratch);
    memset(scratch, 0, sizeof(scratch));
    strncpy(scratch, buf, bufsz - 1);
    int pos = (int)strlen(scratch);
    int ch;
    bool editing = true;

    curs_set(1);
    noecho();
    keypad(w, TRUE);

    while (editing) {
        mvwprintw(w, row, col, "%-*s", (int)bufsz - 1, " ");
        mvwprintw(w, row, col, "%s", scratch);
        wmove(w, row, col + pos);
        wrefresh(w);

        ch = wgetch(w);
        switch (ch) {
            case '\n':
            case KEY_ENTER:
                snprintf(buf, bufsz, "%s", scratch);
                editing = false;
                break;
            case 27:
                editing = false;
                break;
            case KEY_BACKSPACE:
            case 127:
            case 8:
                if (pos > 0) {
                    string_delete_char(scratch, pos - 1);
                    pos--;
                }
                break;
            case KEY_DC:
                if (pos < (int)strlen(scratch)) {
                    string_delete_char(scratch, pos);
                }
                break;
            case KEY_LEFT:
                if (pos > 0) pos--;
                break;
            case KEY_RIGHT:
                if (pos < (int)strlen(scratch)) pos++;
                break;
            case KEY_HOME:
                pos = 0;
                break;
            case KEY_END:
                pos = (int)strlen(scratch);
                break;
            default:
                if (isprint(ch)) {
                    if (strlen(scratch) < bufsz - 1) {
                        string_insert_char(scratch, pos, (int)bufsz, (char)ch);
                        pos++;
                    }
                }
                break;
        }
    }
    curs_set(0);
}

static void edit_double(WINDOW *w, int row, int col, double *val) {
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%.3f", *val);
    edit_string(w, row, col, tmp, sizeof(tmp));
    char *endptr;
    double v = strtod(tmp, &endptr);
    if (endptr != tmp && *endptr == '\0') {
        *val = v;
    }
}

static void draw_form(WINDOW *w, const struct cfg *c, int highlight) {
    werase(w);
    box(w, 0, 0);
    mvwprintw(w, 1, 2, "%s", TITLE);
    mvwprintw(w, 2, 2, "w/s: Move | Enter: Edit/Save | a/d: Cycle Color | Q: Quit");

    for (int i = 0; i < FI_COUNT; ++i) {
        int current_row = UI_START_ROW + i * UI_ROW_STEP;
        if (i == highlight) wattron(w, A_REVERSE);
        
        if (i == FI_SAVE) {
            mvwprintw(w, current_row, UI_LABEL_COL, "[ %s ]", UI_FIELDS[i]);
        } else {
            mvwprintw(w, current_row, UI_LABEL_COL, "%s:", UI_FIELDS[i]);
        }
        
        wattroff(w, A_REVERSE);

        if (i == FI_OUTPUT) {
            mvwprintw(w, current_row, UI_VALUE_COL, "%s", c->output[0] ? c->output : "(empty)");
        } else if (i == FI_GAMMA_MIN) {
            mvwprintw(w, current_row, UI_VALUE_COL, "%.3f", c->gamma_min);
        } else if (i == FI_GAMMA_MAX) {
            mvwprintw(w, current_row, UI_VALUE_COL, "%.3f", c->gamma_max);
        } else if (i == FI_BRIGHT_MIN) {
            mvwprintw(w, current_row, UI_VALUE_COL, "%.3f", c->bright_min);
        } else if (i == FI_BRIGHT_MAX) {
            mvwprintw(w, current_row, UI_VALUE_COL, "%.3f", c->bright_max);
        } else if (i == FI_SEL_COLOR) {
            int idx = c->selected_color & 7;
            mvwprintw(w, current_row, UI_VALUE_COL, "%s", COLOR_NAMES[idx]);
        } else if (i == FI_UNSEL_COLOR) {
            int idx = c->unselected_color & 7;
            mvwprintw(w, current_row, UI_VALUE_COL, "%s", COLOR_NAMES[idx]);
        }
    }
    wrefresh(w);
}

int main(int argc, char **argv) {
    char cfgpath[PATH_MAX];
    const char *exe_name = (argc > 0) ? argv[0] : "gammatui";
    if (!config_path_for_exe(cfgpath, sizeof(cfgpath), exe_name)) {
        fprintf(stderr, "Unable to determine config path.\n");
        return 1;
    }

    struct cfg cur;
    if (!load_config(&cur, cfgpath)) {
        memset(&cur, 0, sizeof(cur));
        strncpy(cur.output, "eDP-1", OUTPUT_LEN - 1);
        cur.gamma_min = 0.1;
        cur.gamma_max = 10.0;
        cur.bright_min = 0.1;
        cur.bright_max = 2.0;
        cur.selected_color = 7;
        cur.unselected_color = 5;
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    WINDOW *win = newwin(rows - 2, cols - 2, 1, 1);
    keypad(win, TRUE);

    int highlight = 0;
    bool running = true;
    bool dirty = true;
    while (running) {
        if (dirty) {
            draw_form(win, &cur, highlight);
            dirty = false;
        }
        int ch = wgetch(win);
        switch (ch) {
            case 'w':
            case 'W':
                if (highlight > 0) { highlight--; dirty = true; }
                break;
            case 's':
            case 'S':
                if (highlight < FI_COUNT - 1) { highlight++; dirty = true; }
                break;
            case 'a':
            case 'A':
                if (highlight == FI_SEL_COLOR) {
                    cur.selected_color = (cur.selected_color + 7) % 8;
                    dirty = true;
                } else if (highlight == FI_UNSEL_COLOR) {
                    cur.unselected_color = (cur.unselected_color + 7) % 8;
                    dirty = true;
                }
                break;
            case 'd':
            case 'D':
                if (highlight == FI_SEL_COLOR) {
                    cur.selected_color = (cur.selected_color + 1) % 8;
                    dirty = true;
                } else if (highlight == FI_UNSEL_COLOR) {
                    cur.unselected_color = (cur.unselected_color + 1) % 8;
                    dirty = true;
                }
                break;
            case '\n':
            case KEY_ENTER: {
                int row = UI_START_ROW + highlight * UI_ROW_STEP;
                if (highlight == FI_SAVE) {
                    if (save_config(&cur, cfgpath)) {
                        show_message(win, "Saved", "Configuration saved successfully.");
                    } else {
                        show_message(win, "Error", "Failed to save configuration.");
                    }
                } else if (highlight == FI_OUTPUT) {
                    edit_string(win, row, UI_VALUE_COL, cur.output, sizeof(cur.output));
                } else if (highlight == FI_GAMMA_MIN) {
                    edit_double(win, row, UI_VALUE_COL, &cur.gamma_min);
                } else if (highlight == FI_GAMMA_MAX) {
                    edit_double(win, row, UI_VALUE_COL, &cur.gamma_max);
                } else if (highlight == FI_BRIGHT_MIN) {
                    edit_double(win, row, UI_VALUE_COL, &cur.bright_min);
                } else if (highlight == FI_BRIGHT_MAX) {
                    edit_double(win, row, UI_VALUE_COL, &cur.bright_max);
                }
                dirty = true;
                break;
            }
            case 'q':
            case 'Q':
                running = false;
                break;
            case KEY_RESIZE:
                getmaxyx(stdscr, rows, cols);
                wresize(win, rows - 2, cols - 2);
                mvwin(win, 1, 1);
                dirty = true;
                break;
        }
    }
    delwin(win);
    endwin();
    return 0;
}
