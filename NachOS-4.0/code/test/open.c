#include "syscall.h"

int main(){
    
    // int id; 
    int c1 = Create("test2.txt");
    int c2 = Create("test3.txt");
    int c3 = Create("test4.txt");
   
    int id = Open("test.txt", 1);
    int id1 = Open("test2.txt", 1);
    int id2 = Open("test3.txt", 1);
    int id3 = Open("test4.txt", 1);

    int res = Close(id);
    int res1 = Close(id1);
    int res2 = Close(id2);
    int res3 = Close(id3);

    Halt();
}