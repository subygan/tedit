#include <iostream>
#include <unistd.h>
#include <termios.h>


struct termios orig_termios;
void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {

    tcgetattr(STDIN_FILENO, &orig_termios); //get terminal attributes

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); //Flip the ECHO and ICANON bits in c_flag

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); //Set the new terminal setting
}

int main() {
    enableRawMode();

    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
        if (iscntrl(c)) {
            printf("%d\n", c);
        } else {
            printf("%d ('%c')\n", c, c);
        }
    };
    return 0;
}