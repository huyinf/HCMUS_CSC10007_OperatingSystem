#include "syscall.h"

int main()
{

  // int createFile = Create("fsdfsdfsdfsd.txt");
  char buff[100];
  int size = 13;
  int id, id1, cnt, res1; 
  
  id = Open("test2.txt", 0);
  if (id >= 2 && id < 20)
  {
      size = Seek(-1, id);
      Seek(0, id);
      cnt = Read(buff, size, id);
  }
  id1 = Open("test3.txt", 0);
  // cnt = Read(buff, size, id);
  // int res = Remove("fsdfsdfsdfsd");
  res1 = Write(buff, cnt, id1);


  Halt();
}