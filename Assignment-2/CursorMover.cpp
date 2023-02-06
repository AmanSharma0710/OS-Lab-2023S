#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#define ctrl(x)           ((x) & 0x1f)
#define max(a, b)         ((a) > (b) ? (a) : (b))
#define min(a, b)         ((a) < (b) ? (a) : (b))
#define ENTER 13
#define START 29
#define END 31
#define BACKSPACE 263
#define DELETE 330
#define UP 259
#define DOWN 258
#define LEFT 260
#define RIGHT 261

int main(void)
{
    int c = 0;
    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    nonl();
    int start = 0;
    int end = 0;
    int pointer = 0;
    int len = 0;
    // press enter to exit
    while(c != ENTER) {
        c = getch();
        switch (c) {
            case ENTER: 
                break;
            case START:
                move(0, start);
                pointer = 0;
                break;
            case END:
                move(0, end);
                pointer = end;
                break;
            
            case BACKSPACE:
                if (end == 0 || pointer == 0) {
                    break;
                }
                end--;
                if (pointer > end) {
                    pointer = end;
                }
                move(0, pointer);
                delch();

                break;

            case DELETE:
                if (end == pointer) {
                    break;
                }
                move(0, pointer);
                delch();
                end--;
                break;
            
            case LEFT:
                pointer = max(0, pointer - 1);
                move(0, pointer);
                break;
            case RIGHT:
                pointer = min(end, pointer + 1);
                move(0, pointer);
                break;
            
            default:
                printw("%c", c);
                pointer++;
                if (pointer > end) {
                    end++;
                }
                break;
        }
    }
    endwin();
    return 0;

    
}