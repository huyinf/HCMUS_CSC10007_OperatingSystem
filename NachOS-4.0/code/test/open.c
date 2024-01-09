#include "syscall.h"

int main(){

    int res3 = Create("011.txt");
    int res4 = Create("021.txt");
    int res5 = Create("031.txt");
    int res6 = Create("041.txt");

    int id = Open("011.txt", 0);
    int id1 = Open("5.txt", 1);
    int id2 = Open("6.txt", 1);
    int id3 = Open("7.txt", 1);



    int res = Close(id);
    int res1 = Close(id1);
    int res2 = Close(id2);
    int res0 = Close(id3);

    int r1 = Remove("a.txt");
    // int r2 = Remove("021.txt");
    int r3 = Remove("031.txt");
    int r4 = Remove("041.txt");
    int sc = SocketTCP();

    Halt();
}