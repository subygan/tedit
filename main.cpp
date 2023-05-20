#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <iostream>
#include <execinfo.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <utility>
#include <vector>
#include <fstream>


#define KILO_VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {(char *)"", 0}

using namespace std;

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

typedef struct erow {
    size_t size;
    char *chars;
} erow;

struct editorConfig {
    int cx, cy;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int numrows;
    vector<erow *> row;
    struct termios orig_termios;
};

struct editorConfig E;

/*** append buffer to append strings together ***/
struct abuf {
    char *b;
    int len;
};


void die(const char *s) {
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
        die("tcsetattr");
    }
}

void enableRawMode() {

    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = E.orig_termios;

    // Turning off flags to go into Raw mode
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr"); //Set the new terminal setting

}

template<typename Type>
void logg(Type s) {
    ofstream myfile;
    myfile.open ("output.txt",std::ios_base::app);
    myfile << s << "::" << typeid(s).name() <<endl;

    myfile.close();
}


void abAppend(struct abuf *ab, char *s, int len) {

    // Allocate additional memory if necessary
//    char *new_b = new char[len];
    char *new_b;
    new_b = static_cast<char *>(malloc(ab->len + len + 1));
    if (new_b == NULL) {
        fprintf(stderr, "Error: unable to allocate memory\n");
        exit(1);
    }


    ::strcpy(new_b, ab->b);
    ::strcat(new_b, s);
//    // Copy new characters to the end of the buffer
//    std::memcpy(new_b + ab->len, s, len);
//    // Update struct fields
    ab->b = new_b;
    ab->len += len;


//    char *arr = new char[ab->len + len];
//
//    string n = (char *) realloc(ab->b, ab->len + len);
//    printf("%s, %s", n, ab->b.c_str());
//
//    if (n == NULL) return;
//    abuf->
//    memcpy(&n[ab->len], s, len);
//    ab->b = n;
//    ab->len += len;
}

void abFree(struct abuf *ab) {
    ab->b = (char *)"";
}

void editorScroll() {
    if (E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if (E.cx < E.coloff) {
        E.coloff = E.cx;
    }
    if (E.cx >= E.coloff + E.screencols) {
        E.coloff = E.cx - E.screencols + 1;
    }
}


int editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {

        if (nread == -1 && errno != EAGAIN) die("read");
    }

    // handling arrow key movements,
    // Arrow keys are of the form '\x1b[A', '\x1b[B', '\x1b[C', '\x1b[D'
    // We're mapping it back to the asd
    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1':
                            return HOME_KEY;
                        case '3':
                            return DEL_KEY;
                        case '4':
                            return END_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                        case '7':
                            return HOME_KEY;
                        case '8':
                            return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A':
                        return ARROW_UP;
                    case 'B':
                        return ARROW_DOWN;
                    case 'C':
                        return ARROW_RIGHT;
                    case 'D':
                        return ARROW_LEFT;
                    case 'H':
                        return HOME_KEY;
                    case 'F':
                        return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
            }
        };
        return '\x1b';
    } else {
        return c;
    }
}

void editorMoveCursor(int key) {
    erow * row = reinterpret_cast<erow *>((E.cy >= E.numrows) ? NULL : &E.row[E.cy]);

    switch (key) {
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy]->size;
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->size) {
                E.cx++;
            } else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
        case ARROW_UP:
            if (E.cy != 0) {
                E.cy--;
            }
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows) {
                E.cy++;
            }
            break;
    }
    row = reinterpret_cast<erow *>((E.cy >= E.numrows) ? NULL : &E.row[E.cy]);
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}

/*** input ***/
void editorProcessKeypress() {

    int c = editorReadKey();
    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            E.cx = E.screencols - 1;
            break;
        case PAGE_UP:
        case PAGE_DOWN: {
            int times = E.screenrows;
            while (times--)
                editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        }
            break;
        case ARROW_LEFT:
        case ARROW_RIGHT:
        case ARROW_UP:
        case ARROW_DOWN:
            editorMoveCursor(c);
            break;
    }
}

/*** row operations ***/
void editorAppendRow(char *s, size_t len) {
//    E.row = (erow*) realloc(E.row, sizeof(erow) * (E.numrows + 1));

//    int at = E.numrows;

//    E.row[at]->size = len;
//    E.row[at]->chars = (char *)malloc(len + 1);
//    memcpy(E.row[at]->chars, s, len);
//    E.row[at]->chars[len] = '\0';

    logg(s);
    int at = E.numrows;
    erow * er = new erow;
    er->size = len;
    er->chars = new char[len] ;
    E.row.push_back(er);
    ::strcpy(E.row[at]->chars, s);
    E.row[at]->chars[len] = '\0';
    logg("@@@");
    logg(E.row.back()->chars);
    logg(E.row.size());
    logg(sizeof(E.row.back()->chars));
    E.numrows++;
}


/*** file i/o ***/
void editorOpen(char *filename) {

    // Opening a file and getting a file pointer.
    // Then reading them one by one.
    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");
    char *line = nullptr;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            linelen--;

        editorAppendRow(line, linelen);
        logg("^^^");
//        logg(E.row.back()->chars);
    }
    logg("E.numrows");
    logg(E.numrows);
    logg(E.row.size());
    if (E.row.back()->chars == nullptr ){
        logg("nullptr!!!");
    } else {
        logg("not null");
    }
//    cout<<std::strlen(E.row.back()->chars);
    free(line);
    fclose(fp);
}

void editorDrawRows(struct abuf *ab) {
    int y;
    logg("screenrows: ");
    logg(E.screenrows);
    logg(E.rowoff);
    logg(E.numrows);
    for (y = 0; y < E.screenrows; y++) {
        int filerow = y + E.rowoff;
        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                      "tedit --version %s", KILO_VERSION);
                if (welcomelen > E.screencols) welcomelen = E.screencols;
                int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    abAppend(ab, (char *)"~", 1);
                    padding--;
                }
                while (padding--) abAppend(ab, (char *)" ", 1);
                abAppend(ab, welcome, welcomelen);
            } else {
                abAppend(ab, (char *)"~", 1);
            }
        } else {
            int len = E.row[filerow]->size - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screencols) len = E.screencols;
            logg("abAppend::");
            logg(filerow);
            logg(E.row[filerow]->chars);
            logg(&E.row[filerow]->chars[E.coloff]);
            abAppend(ab, &E.row[filerow]->chars[E.coloff], len);
        }
        abAppend(ab, (char *)"\x1b[K", 3);

        if (y < E.screenrows - 1) {
            abAppend(ab, (char *)"\r\n", 2);
        }
    }
    logg(ab->b);
    logg("ab->b");
}


// Refresh screen,
void editorRefreshScreen() {
    logg("!!!!!!!!!!!!!!!!!");
    editorScroll();


    struct abuf ab = ABUF_INIT;

    abAppend(&ab, (char *)"\x1b[?25l", 6);
    abAppend(&ab, (char *)"\x1b[H", 3);

    logg(ab.b);
    logg("!!!!!!!!!!!!!!!!!");

    editorDrawRows(&ab);

    logg(ab.b);
    logg("-------------------");
    char buf[32];

    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.cx - E.coloff) + 1);


    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, (char *)"\x1b[?25h", 6);
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

int getCursorPosition(int *rows, int *cols) {

    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
    return 0;
}


int getWindowSize(int *rows, int *cols) {

    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }

}

void initEditor() {
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = vector<erow *>();
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

void error_handler(int sig) {
    disableRawMode();
    void *array[10];
    size_t size;
    size = backtrace(array, 10); //get the void pointers for all of the entries
//    cout << "Error: signal "<< sig <<":\n"; //display error signal
    backtrace_symbols_fd(array, size, STDERR_FILENO);
}



int main(int argc, char *argv[]) {
    signal(SIGSEGV, error_handler);
    enableRawMode();
    initEditor();
    logg("#####");
    if (argc >= 2) {
        editorOpen(argv[1]);
    }

    while (1) {
        logg("!!!!!");
        editorRefreshScreen();
        editorProcessKeypress();
    };
    return 0;
}