#include <iostream>
#include <unistd.h>

int main() {
    // Write C++ code here
    std::cout << "Hello world!";
    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    return 0;
}