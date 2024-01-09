#include "../userprog/syscall.h"

int main() {
    if(Remove("0.txt") == 0){
        PrintString("Delete File Successfully!");
    }
    else{
        PrintString("Delete File not Successfully!");
    }
    if(Remove("1.txt") == 0){
        PrintString("Delete File Successfully!");
    }
    else{
        PrintString("Delete File not Successfully!");
    }
    if(Remove("2.txt") == 0){
        PrintString("Delete File Successfully!");
    }
    else{
        PrintString("Delete File not Successfully!");
    }
    if(Remove("3.txt") == 0){
        PrintString("Delete File Successfully!");
    }
    else{
        PrintString("Delete File not Successfully!");
    }
    
    Halt();
}