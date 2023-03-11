#include "iostream"
using namespace std;

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
};

int main() {
    cout << ARROW_LEFT << endl;
    cout << ARROW_RIGHT << endl;
    cout << ARROW_UP << endl;
    cout << ARROW_DOWN << endl;

    return 0;
}