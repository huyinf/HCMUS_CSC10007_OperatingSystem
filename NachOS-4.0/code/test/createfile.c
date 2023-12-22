#include "syscall.h"

int main(){
    int res = Create("test.txt");
    Halt();
}