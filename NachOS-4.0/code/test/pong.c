/*
    Test semaphore and multithread
    print 1000 character 'B'
*/

#include "syscall.h"

int main() {
    int pongTest = CreateSemaphore("pong",1);
    int i;
    for (i = 0; i < 10; ++i) {
        Signal("ping");
        // PrintChar('B');
        PrintString("\nB\n");
        Wait("pong");

    }
    Exit(0);
}