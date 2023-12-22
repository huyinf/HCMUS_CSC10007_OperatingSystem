#include "../userprog/syscall.h"

int main() {
    if(Remove("test.txt") == 0){
        PrintString("Delete File Successfully!");
    }
    else{
        PrintString("Delete File not Successfully!");
    }
    
    Halt();
}