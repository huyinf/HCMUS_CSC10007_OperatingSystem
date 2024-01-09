#include "syscall.h"

int main(){
    int res = Create("011.txt");
    res = Create("11.txt");
	res = Create("22.txt");
	res = Create("32.txt");
    Halt();
}