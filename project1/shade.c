#include "shade.h"

int shade(char c, char *cp) {
    switch(c) {
        case '0':
            *cp = ' ';
            break;
        case '1':
            *cp = '.';
            break;
        case '2':
            *cp = ':';
            break;
        case '3':
            *cp = 'c';
            break;
        case '4':
            *cp = 'o';
            break;
        case '5':
            *cp = 'C';
            break;
        case '6':
            *cp = 'O';
            break;
        case '7':
            *cp = '8';
            break;
        case '8':
            *cp = '@';
            break;
        case '\n':
            break;
        case '\r':
            break;
        default:
            return 0;
    }
    return 1;
}