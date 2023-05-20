#include <iostream>

struct myStruct {
    size_t size;
    char *chars;
};
struct myStruct M;

void func1(){
    M.size = 0;
    char c[] = "something";
    M.chars = &c[0];
    std::cout << M.chars<<std::endl;
}

int main(){
    func1();
    std::cout<<M.chars<<std::endl;
    return 0;
}
