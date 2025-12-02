#define _POSIX_C_SOURCE 200809L
#include "settings.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#define K(x) ((x)>='A'&&(x)<='Z'?(x)+32:(x))

int main(int c, char **v) {
    struct cfg g;
    char p[4096], b[32];
    struct { char *l; int t; void *v; } f[] = {
        {"Output", 0, g.output},
        {"Gamma Min", 1, &g.gamma_min}, {"Gamma Max", 1, &g.gamma_max},
        {"Bright Min", 1, &g.bright_min}, {"Bright Max", 1, &g.bright_max},
        {"Sel R", 2, &g.sel_r}, {"Sel G", 2, &g.sel_g}, {"Sel B", 2, &g.sel_b},
        {"Unsel R", 2, &g.unsel_r}, {"Unsel G", 2, &g.unsel_g}, {"Unsel B", 2, &g.unsel_b},
        {"Key Up", 3, &g.key_up}, {"Key Down", 3, &g.key_down},
        {"Key Sel", 3, &g.key_select}, {"Key Quit", 3, &g.key_quit},
        {"Save", 4, 0}
    };
    int r, co, i, k, h = 0, run = 1, cnt = 16;

    if (!config_path_for_exe(p, sizeof(p), c > 0 ? v[0] : 0) || !load_config(&g, p))
        config_set_defaults(&g);

    initscr(); cbreak(); noecho(); keypad(stdscr, 1); curs_set(0);
    WINDOW *w = newwin(0, 0, 0, 0);

    while (run) {
        getmaxyx(stdscr, r, co);
        wresize(w, r - 2, co - 2); mvwin(w, 1, 1);
        werase(w); box(w, 0, 0);
        mvwprintw(w, 1, 2, "Settings");

        for (i = 0; i < cnt; i++) {
            if (i == h) wattron(w, A_REVERSE);
            mvwprintw(w, 3 + i, 2, "%-12s", f[i].l);
            wmove(w, 3 + i, 15);
            if (f[i].t == 0) waddstr(w, f[i].v);
            else if (f[i].t == 1) wprintw(w, "%.2f", *(double*)f[i].v);
            else if (f[i].t == 2) wprintw(w, "%d", *(int*)f[i].v);
            else if (f[i].t == 3) waddstr(w, keyname(*(int*)f[i].v));
            if (i == h) wattroff(w, A_REVERSE);
        }
        
        if ((k = wgetch(w)) == ERR) continue;
        int nk = K(k);

        if (nk == K(g.key_up)) h = (h ? h : cnt) - 1;
        else if (nk == K(g.key_down)) h = (h + 1) % cnt;
        else if (nk == K(g.key_quit)) run = 0;
        else if (nk == K(g.key_select) || k == 10) {
            int t = f[h].t;
            mvwprintw(w, 3 + h, 15, "              ");
            
            if (t == 4) {
                mvwaddstr(w, 3 + h, 15, save_config(&g, p) ? "Saved" : "Error");
                wrefresh(w); napms(500);
            } else if (t == 3) {
                curs_set(1); mvwaddstr(w, 3 + h, 15, "Press..."); wrefresh(w);
                if ((k = wgetch(w)) != 27 && k != ERR) {
                    int ok = 1;
                    for(i=0; i<cnt; i++) if(f[i].t==3 && i!=h && K(*(int*)f[i].v)==K(k)) ok=0;
                    if(ok) *(int*)f[h].v = k;
                }
                curs_set(0);
            } else {
                curs_set(1); echo();
                mvwgetnstr(w, 3 + h, 15, b, 31);
                noecho(); curs_set(0);
                if (*b) {
                    if (t == 0) strncpy(f[h].v, b, 127);
                    else if (t == 1) *(double*)f[h].v = strtod(b, 0);
                    else *(int*)f[h].v = atoi(b);
                }
            }
        }
    }
    delwin(w); endwin();
    return 0;
}
