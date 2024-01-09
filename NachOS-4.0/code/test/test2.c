#include "syscall.h"

int main (int argc, char* argv[]){

SpaceId newProc1;
SpaceId newProc2;
int catTest = CreateSemaphore("cat",1);
int openTest = CreateSemaphore("open",1);
newProc1 = Exec("cat"); // Project 01
newProc2 = Exec("open"); // Project 01
// newProc1 = ExecV(argc,argv[1]);
// newProc2 = ExecV(argc,argv[2]);
Join(newProc1);
Join(newProc2);
}