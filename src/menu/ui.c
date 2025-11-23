#define _POSIX_C_SOURCE 200809L
#include "ui.h"
#include <ncurses.h>
#include <string.h>

const char *MENU_TITLE = "gammatui - Main Menu";
const char *menu_items[] = { "Adjustment", "Settings", "Quit" };

void draw_menu(WINDOW *w,int hl){
    if(!w) return;
    werase(w); box(w,0,0);
    mvwprintw(w,1,2,"%s",MENU_TITLE);
    mvwprintw(w,2,2,"Use w/s to move, Enter to select");
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
