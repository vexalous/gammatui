#define _POSIX_C_SOURCE 200809L
#include "ui.h"
#include <ncurses.h>
#include <string.h>

const char *MENU_TITLE = "gammatui - Main Menu";
const char *menu_items[] = { "Adjustment", "Settings", "Quit" };

void draw_menu(WINDOW *w, int hl, int k_up, int k_down, int k_sel, int k_quit){
    if(!w) return;
    werase(w); box(w,0,0);
    mvwprintw(w,1,2,"%s",MENU_TITLE);

    const char *kn_up = keyname(k_up); if(!kn_up) kn_up="?";
    const char *kn_dn = keyname(k_down); if(!kn_dn) kn_dn="?";
    const char *kn_sl = keyname(k_sel); if(!kn_sl) kn_sl="?";
    const char *kn_qt = keyname(k_quit); if(!kn_qt) kn_qt="?";

    char help[128];
    snprintf(help, sizeof(help), "Use %s/%s to move, %s to select, %s to quit", kn_up, kn_dn, kn_sl, kn_qt);
    mvwprintw(w, 2, 2, "%s", help);

    for(int i=0,y=5;i<3;++i){
        if(i==hl) wattron(w,A_REVERSE|A_BOLD);
        mvwprintw(w,y+i,4,"%s",menu_items[i]);
        if(i==hl) wattroff(w,A_REVERSE|A_BOLD);
    }
    wnoutrefresh(w); doupdate();
}

void show_message(WINDOW *w,const char *t,const char *m){
    if(!w) return;
    werase(w); box(w,0,0);
    mvwprintw(w,1,2,"%s",t);
    mvwprintw(w,3,2,"%s",m);
    mvwprintw(w,5,2,"Press any key to return");
    wnoutrefresh(w); doupdate();
    wgetch(w);
}

WINDOW *recreate_window(int *rows,int *cols){
    if (stdscr == NULL) initscr();
    clear(); refresh();
    getmaxyx(stdscr,*rows,*cols);
    WINDOW *w = newwin(*rows - 2, *cols - 2, 1, 1);
    keypad(w, TRUE); wtimeout(w, -1);
    return w;
}
