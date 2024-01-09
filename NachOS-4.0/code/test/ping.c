#include "syscall.h"

int main() {
    int pingTest = CreateSemaphore("ping",1);
    int i;
    for (i = 0; i < 10; i++) {
        Wait("ping");
        // PrintChar('A');
        PrintString("\nA\n");
        Signal("pong");
    }
    Exit(0);
}