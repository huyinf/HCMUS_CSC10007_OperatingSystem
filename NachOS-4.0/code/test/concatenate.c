#include "../userprog/syscall.h"

int main()
{
	OpenFileId id1;
	OpenFileId id2;
	OpenFileId id3;
	int size1, size2;
	int cnt1, cnt2;
	char *buffer1;
	char *buffer2;
	int res1, res2;

	id1 = Open("1.txt", 0);
	id2 = Open("2.txt", 0);
	id3 = Open("3.txt", 0);

	// Read(buffer1 ,10 ,id1);
	if (id1 >= 2 && id1 < 20)
	{
		size1 = Seek(-1, id1);
		Seek(0, id1);
		cnt1 = Read(buffer1, size1, id1);
	}
	res1 = Write(buffer1, cnt1, id3);

	// Read(buffer2, 10, id2);
	if (id2 >= 2 && id2 < 20)
	{
		size2 = Seek(-1, id2);
		Seek(0, id2);
		cnt2 = Read(buffer2, size2, id2);
	}
	res2 = Write(buffer2, cnt2, id3);

	if(res1 != -1 && res2 != -1){
		PrintString("Concatenate Successfully!");
	}
	else{
		PrintString("Concatenate not Successfully!");
	}

	Close(id1);
	Close(id2);
	Close(id3);

	Halt();
}