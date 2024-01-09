#include "../userprog/syscall.h"


int main()
{
    char *buffer;
    int cnt, size;
    OpenFileId id = Open("0.txt", 0);
    if (id >= 2 && id < 20)
  {
      size = Seek(-1, id);
      Seek(0, id);
      cnt = Read(buffer, size, id);
  }
    //Test(buffer);
    PrintString(buffer);
    PrintString("\n");
    Close(id);

    Halt();
} 