#include "../userprog/syscall.h"

int main()
{
  OpenFileId id1;
  OpenFileId id2;
  int size, cnt;
  char *buffer;
  id1 = Open("2.txt", 0);

  // Read(buffer, 10,id1);
  if (id1 >= 2 && id1 < 20)
  {
    size = Seek(-1, id1);
    Seek(0, id1);
    cnt = Read(buffer, size, id1);
  }
  id2 = Open("3.txt", 0);
  if (Write(buffer, cnt, id2)){
    PrintString("Copy succesfully!");
  }
  else{
    PrintString("Copy not succesfully!");
  }
  Close(id1);
  Close(id2);

  Halt();
  return 0;
}