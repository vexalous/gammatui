#define _XOPEN_SOURCE 700
#include "gammatui.h"
#include "gamma_control.h"
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ncurses.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

static struct cfg c;

static void setup_config(const char *arg0) {
    char path[PATH_MAX];
    if (config_path_for_exe(path, sizeof(path), arg0) && load_config(&c, path)) return;
    config_set_defaults(&c);
    if (*path) save_config(&c, path);
}

int main(int argc, char **argv) {
    setup_config(argc > 0 ? argv[0] : NULL);

    int opt;
    while ((opt = getopt(argc, argv, "ho:")) != -1) {
        if (opt == 'o') snprintf(display_output, sizeof(display_output), "%s", optarg);
        else return printf("Usage: %s [--output NAME]\n", argv[0]), opt == 'h' ? 0 : 1;
    }

    if (!*display_output && !detect_output(display_output, sizeof(display_output))) 
        *display_output = '\0';

    if (system("command -v xrandr >/dev/null 2>&1"))
        fprintf(stderr, "WARNING: xrandr not found.\n");

    double vals[2] = {
        clamp_double(1.0, c.gamma_min, c.gamma_max),
        clamp_double(1.0, c.bright_min, c.bright_max)
    };
    int sel = 0, rows, cols;
    int keys[] = { tolower(c.key_up), tolower(c.key_down), tolower(c.key_select), tolower(c.key_quit) };

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    if (has_colors()) 
        init_colors_safe(c.sel_r, c.sel_g, c.sel_b, c.unsel_r, c.unsel_g, c.unsel_b);

    getmaxyx(stdscr, rows, cols);
    if (rows < 5 || cols < 10) {
        endwin();
        return fprintf(stderr, "Terminal too small.\n"), 1;
    }

    WINDOW *win = newwin(rows - 2, cols - 2, 1, 1);
    keypad(win, TRUE);

    int ch;
    bool running = true;
    
    do {
        draw_ui(win, vals[0], vals[1], sel, rows - 2, cols - 2, c.key_up, c.key_down, c.key_select, c.key_quit);
        
        if ((ch = wgetch(win)) == ERR) break;
        
        int key = tolower(ch);
        bool redraw = false, apply = false;

        if (key == keys[3]) running = false;
        else if (key == keys[0]) { sel = 0; redraw = true; }
        else if (key == keys[1]) { sel = 1; redraw = true; }
        else if (key == keys[2]) {
            char buf[32];
            curs_set(1); echo();
            mvwprintw(win, rows - 4, 2, "Set Value: ");
            wrefresh(win);
            wgetnstr(win, buf, sizeof(buf) - 1);
            noecho(); curs_set(0);
            
            char *end;
            double v = strtod(buf, &end);
            double min = sel ? c.bright_min : c.gamma_min;
            double max = sel ? c.bright_max : c.gamma_max;
            
            if (end != buf && v >= min && v <= max) {
                vals[sel] = v;
                apply = true;
            }
            redraw = true;
        } else if (key == 'r') {
            vals[0] = clamp_double(1.0, c.gamma_min, c.gamma_max);
            vals[1] = clamp_double(1.0, c.bright_min, c.bright_max);
            apply = redraw = true;
        } else if (ch == KEY_RESIZE) {
            getmaxyx(stdscr, rows, cols);
            if (rows >= 5 && cols >= 10) {
                wresize(win, rows - 2, cols - 2);
                mvwin(win, 1, 1);
                clear(); refresh();
                redraw = true;
            }
        }

        if (apply && debounce_allow()) apply_values(vals[0], vals[1]);

    } while (running);

    delwin(win);
    endwin();
    return 0;
