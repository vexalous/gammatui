#define _XOPEN_SOURCE 700
#include "gammatui.h"
#include "gamma_control.h"
#include "settings.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <ncurses.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

static struct cfg c;

int main(int argc, char **argv) {
    char path[PATH_MAX], buf[32], *end;
    int opt, rows, cols, sel = 0, ch, k_up, k_dn, k_sl, k_qt;
    double vals[2], min, max, v;
    bool running = true, dirty = true, apply;

    setenv("ESCDELAY", "25", 0);

    if (config_path_for_exe(path, sizeof(path), argc > 0 ? argv[0] : NULL) && !load_config(&c, path)) {
        config_set_defaults(&c);
        if (*path) save_config(&c, path);
    }

    while ((opt = getopt(argc, argv, "ho:")) != -1) {
        if (opt == 'o') snprintf(display_output, sizeof(display_output), "%s", optarg);
        else return opt == 'h' ? 0 : 1;
    }

    if (!*display_output && !detect_output(display_output, sizeof(display_output))) *display_output = 0;

    vals[0] = clamp_double(1.0, c.gamma_min, c.gamma_max);
    vals[1] = clamp_double(1.0, c.bright_min, c.bright_max);

    k_up = tolower(c.key_up); k_dn = tolower(c.key_down);
    k_sl = tolower(c.key_select); k_qt = tolower(c.key_quit);

    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
    if (has_colors()) init_colors_safe(c.sel_r, c.sel_g, c.sel_b, c.unsel_r, c.unsel_g, c.unsel_b);

    getmaxyx(stdscr, rows, cols);
    if (rows < 5 || cols < 10) { endwin(); return 1; }
    WINDOW *win = newwin(rows - 2, cols - 2, 1, 1);
    keypad(win, TRUE);

    while (running) {
        if (dirty) {
            draw_ui(win, vals[0], vals[1], sel, rows - 2, cols - 2, c.key_up, c.key_down, c.key_select, c.key_quit);
            dirty = false;
        }

        if ((ch = wgetch(win)) == ERR) break;
        ch = tolower(ch);
        apply = false;
        min = sel ? c.bright_min : c.gamma_min;
        max = sel ? c.bright_max : c.gamma_max;

        if (ch == k_qt) running = false;
        else if (ch == k_up) { if (sel) { sel = 0; dirty = true; } }
        else if (ch == k_dn) { if (!sel) { sel = 1; dirty = true; } }
        else if (ch == k_sl || ch == 10) {
            curs_set(1); echo();
            mvwprintw(win, rows - 4, 2, "Set Value:           ");
            wmove(win, rows - 4, 13);
            wrefresh(win);
            wgetnstr(win, buf, sizeof(buf) - 1);
            noecho(); curs_set(0);
            v = strtod(buf, &end);
            if (end != buf && *end == 0 && v >= min && v <= max) { vals[sel] = v; apply = dirty = true; }
            else dirty = true;
        }
        else if (ch == 'r') { vals[0] = vals[1] = 1.0; apply = dirty = true; }
        else if (ch == KEY_RESIZE) {
            getmaxyx(stdscr, rows, cols);
            if (rows >= 5 && cols >= 10) {
                wresize(win, rows - 2, cols - 2); mvwin(win, 1, 1);
                clear(); refresh(); dirty = true;
            }
        }
        else if (ch == KEY_LEFT || ch == KEY_RIGHT) {
            vals[sel] = clamp_double(vals[sel] + (ch == KEY_LEFT ? -0.05 : 0.05), min, max);
            apply = dirty = true;
        }

        if (apply && debounce_allow()) apply_values(vals[0], vals[1]);
    }

    delwin(win); endwin();
    return 0;
}
