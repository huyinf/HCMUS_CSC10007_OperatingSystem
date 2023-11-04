// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
#include "cstring"
#define MaxFileLength 32
#define MAX_CHARACTERS 100
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------
void IncreasePC()
{
	/* set previous programm counter (debugging only)*/
	kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

	/* set next programm counter for brach execution */
	kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
}

// Copy data tu user space -> kernel space
char *User2System(int virtAddr, int limit)
{
	int i; // chi so index
	int oneChar;
	char *kernelBuf = NULL;
	kernelBuf = new char[limit + 1]; // can cho chuoi terminal
	if (kernelBuf == NULL)
		return kernelBuf;

	memset(kernelBuf, 0, limit + 1); // tao mot mang voi do dai limit + 1

	for (i = 0; i < limit; i++)
	{
		kernel->machine->ReadMem(virtAddr + i, 1, &oneChar);
		kernelBuf[i] = (char)oneChar;
		if (oneChar == 0)
			break;
	}
	return kernelBuf;
}

// Copy data tu user space -> kernel space
int System2User(int virtAddr, int len, char *buffer)
{
	if (len < 0)
		return -1;
	if (len == 0)
		return len;
	int i = 0;
	int oneChar = 0;
	do
	{
		oneChar = (int)buffer[i];
		kernel->machine->WriteMem(virtAddr + i, 1, oneChar);
		i++;
	} while (i < len && oneChar != 0);
	return i;
}

void Create()
{
	int virtAddr;	// tao dia chi
	char *filename; // luu tru string
	DEBUG('u', "\n SC_CreateFile call ...");
	DEBUG('u', "\n Reading virtual address of filename");

	virtAddr = kernel->machine->ReadRegister(4); // Doc dia chi cua file tu thanh ghi R4
	DEBUG('u', "\n Reading filename.");

	filename = User2System(virtAddr, MaxFileLength + 1); // copy data tu user -> kernel de xu ly
	if (strlen(filename) == 0)
	{
		DEBUG('u', "\n File name is not valid");
		kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
		IncreasePC();
		return;
	}
	if (filename == NULL) // Neu khong doc duoc
	{
		// printf("\n Not enough memory in system");
		DEBUG('a', "\n Not enough memory in system");
		kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
		delete[] filename;
		IncreasePC();
		return;
	}
	DEBUG('u', "\n Finish reading filename.");

	if (!kernel->fileSystem->Create(filename)) // Tao file bang ham Create cua fileSystem, tra ve ket qua
	{
		// Tao file that bai
		printf("\n Error create file '%s'", filename);
		kernel->machine->WriteRegister(2, -1);
		delete[] filename;
		IncreasePC();
	}

	// Tao file thanh cong
	// cout << "Create file success" << endl;
	DEBUG('u', "\n Create file success");
	kernel->machine->WriteRegister(2, 0);
	delete[] filename;
	IncreasePC(); // Day thanh ghi lui ve sau de tiep tuc ghi
}

void OpenfileID()
{
	// Input: filename, type (0 for read and write and 1 for read file)
	// Output: failed or -1 for fail
	// case for fail: over 20 files - cannot find file name (not exist)

	int virtAddr = kernel->machine->ReadRegister(4); // Doc thanh ghi 4
	char *filename;
	int type = kernel->machine->ReadRegister(5); // Doc thanh ghi 5
	filename = User2System(virtAddr, MaxFileLength + 1);

	int tempt = kernel->fileSystem->isFull();
	// Check over 20 files
	if (tempt == -1)
	{
		// printf("\n over 20 files are opened");
		DEBUG(dbgSys, "\n over 20 files are opened")
		kernel->machine->WriteRegister(2, -1);
		delete[] filename;
		IncreasePC();
		return;
	}
	// file name không nhận
	if (strlen(filename) == 0)
	{
		// printf("\n File name is not valid");
		DEBUG(dbgSys, "\n File name is not valid");
		kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
		IncreasePC();
		return;
	}
	if (filename == NULL) // Neu khong doc duoc
	{
		// printf("\n Not enough memory in system");
		DEBUG(dbgSys, "\n Not enough memory in system");
		kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
		delete[] filename;
		IncreasePC();
		return;
	}

	// success
	// only open file when type = 1 or type = 0
	if (type == 1 || type == 0)
	{
		kernel->fileSystem->openfile[tempt] = kernel->fileSystem->Open(filename, type);
		if ((kernel->fileSystem->openfile[tempt]) != NULL)
		{
			// cout << "Success open file"
			// 	 << " file id: " << tempt << endl;
			DEBUG(dbgSys, "\n Success open file ");
            DEBUG(dbgSys,"\n opened filename: "<<filename);
			DEBUG(dbgSys, "\n File id: " << tempt);

			// kernel->fileSystem->filename[tempt] = filename;
			kernel->machine->WriteRegister(2, tempt); // index -1 vi after open thì nó đã cộng một vị trí index
													  // cout<<kernel->fileSystem->openfile[kernel->fileSystem->index-1]->type;
		}
		else
		{
			// cout << "Cannot open file" << endl;
			DEBUG(dbgSys, "\nCannot open file")
			kernel->machine->WriteRegister(2, -1);
		}
	}
	else
	{
		cout << "Cannot open file because wrong type" << endl;
		kernel->machine->WriteRegister(2, -1);
	}

	delete[] filename;
	IncreasePC();
	return;
}

void Close()
{
	int id = kernel->machine->ReadRegister(4);
	// neu id < 2 (0,1 danh cho console input,output)
	// hoac id >= 20 (over)
	// -> bao loi, return -1
	if (id < 2 || id >= 20)
	{
		printf("\nInvalid file id\n");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
	}
	// hoac ko mo dc file voi id
	if (kernel->fileSystem->openfile[id] == NULL)
	{
		printf("\nCannot close file \n");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
	}
	// return 0 if success
	kernel->machine->WriteRegister(2, 0);
	kernel->fileSystem->filename[id] = "";
	delete kernel->fileSystem->openfile[id];
	kernel->fileSystem->openfile[id] = NULL;
	// dung ham printf gap loi kieu du lieu string nen da them c_strc()
	printf("%s\n", kernel->fileSystem->filename[id].c_str());
	printf("\nClose file successfully");
	IncreasePC();
}

void Read()
{
	// input: buffer (char*), size(int), id(OpenFileID)
	DEBUG(dbgSys, "\n SC_Read call ...");
	int virtAddr = kernel->machine->ReadRegister(4);
	int charcount = kernel->machine->ReadRegister(5);
	int id = kernel->machine->ReadRegister(6);

	int OldPos;
	int NewPos;
	char *buffer;
	// int tmp;
	// kiem tra id co vuot gioi han 20 files
	if (id < 0 || id >= 20)
	{
		DEBUG(dbgSys, "\nCannot read file because id is out of range.");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
	}
	// check if file is existed
	if (kernel->fileSystem->openfile[id] == NULL)
	{
		DEBUG(dbgSys, "\nCannot read file because not exist.");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
		// break;
	}
	// id==1 --> console stdout, this file cannot be read, return -1
	if (id == 1)
	{
		DEBUG(dbgSys, "\nCannot read file stdout.");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
		// break;
	}
	// neu khong co loi
	// kiem tra vi tri hien tai cua file
	// them phuong thuc GetCurrentPos vao class OpenFile
	OldPos = kernel->fileSystem->openfile[id]->GetCurrentPos();
	buffer = User2System(virtAddr, charcount);
	// id==0, file stdin
	if (id == 0)
	{
		DEBUG(dbgSys,"\nread stdin file");
		// su dung ham read cua lop SynchConsole de tra ve so byte thuc su doc duoc
		int size = 0;

		char* newBuff = new char[MAX_CHARACTERS];
		memset(newBuff,0,MAX_CHARACTERS);

		char c = kernel->ReadChar();
		// if character is \n, newline, enter
		int cnt = 0;
		while(c != (char)10){
			size++;
			newBuff[cnt] = c;

			if(size > MAX_CHARACTERS){
				DEBUG(dbgSys,"\n Overflow");
				break;
			}
			cnt++;
			c = kernel->ReadChar();

		}
		DEBUG(dbgSys,"\nsize = "<<size);

		buffer[size] = '\0';

		// copy chuoi buffer tu vung nho System Space vao User Space
		// voi bo dem buffer la so byte thuc su
		System2User(virtAddr, size, buffer);
		// buffer duoc truyen vao se mang chuoi da doc

		// tra ve so byte thuc su doc duoc
		DEBUG('u', "\n Size: " << size);
		kernel->machine->WriteRegister(2, size);

		delete[] buffer;
		IncreasePC();
		return;
		// break;
	}
	// normal file, return true bytes of file
	if ((kernel->fileSystem->openfile[id]->Read(buffer, charcount)) > 0)
	{
		NewPos = kernel->fileSystem->openfile[id]->GetCurrentPos();
		// true bytes = NewPos-OldPos
		System2User(virtAddr, NewPos - OldPos, buffer);
		DEBUG('u', "\n Size: " << NewPos - OldPos);
		kernel->machine->WriteRegister(2, NewPos - OldPos);
	}
	// file content is NULL
	
	else
	{
		DEBUG('u', "\nEmpty file.");
		kernel->machine->WriteRegister(2, -1);
	}
	DEBUG(dbgSys, "\n SC_Read call 1 ...");

	delete[] buffer;
	IncreasePC();
	return;
	// con doan code cho socket --> to be continued
}

void Write()
{
	int virtAddr = kernel->machine->ReadRegister(4);
	int charcount = kernel->machine->ReadRegister(5);
	int id = kernel->machine->ReadRegister(6);
	int OldPos;
	int NewPos;
	// int tempt;
	char *buffer;
	// Kiem tra id cua file truyen vao co nam ngoai bang mo ta file khong
	if (id < 0 || id >= 20)
	{
		DEBUG(dbgSys, "\nCannot write file because id is out of range.");
		DEBUG(dbgSys, "id: " << id);
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
	}
	// Kiem tra file co ton tai khong
	if (kernel->fileSystem->openfile[id] == NULL)
	{
		DEBUG(dbgSys, "\nCannot write file because file is not existed.");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
	}
	// ghi file only read (type quy uoc la 1)
	// hoac file stdin thi tra ve -1
	if (id == 0 || kernel->fileSystem->openfile[id]->type == 1)
	{
		DEBUG('u', "\nCannot write file stdin or read only file.");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
	}

	// thanh cong
	// lay vi tri OldPos
	OldPos = kernel->fileSystem->openfile[id]->GetCurrentPos();
	// Copy chuoi tu vung nho User Space sang System Space
	// voi bo dem buffer co do dai charcount
	buffer = User2System(virtAddr, charcount);

	// ghi file read & write (type quy uoc la 0) thi tra ve so byte thuc su
	if (kernel->fileSystem->openfile[id]->type == 0)
	{
		if ((kernel->fileSystem->openfile[id]->Write(buffer, charcount)) > 0)
		{
			NewPos = kernel->fileSystem->openfile[id]->GetCurrentPos();
            DEBUG(dbgSys,"\n Write file success.");
            DEBUG(dbgSys,"\n size = "<< NewPos - OldPos);
			kernel->machine->WriteRegister(2, NewPos - OldPos);
			delete[] buffer;
			IncreasePC();
			return;
		}
	}

	// Xet truong hop con lai ghi file stdout
	if (id == 1)
	{
            DEBUG(dbgSys,"\n Write file id = 1.");

        int size = 0;
		int lengthWrite = 0;
		// dung -> vi tri chuoi bang 0
		while (buffer[lengthWrite] != 0)
		{
			kernel->putChar(buffer[lengthWrite]);
            size++;
        	lengthWrite++;
			// Truong hop chuoi vuot qua 255
			if (lengthWrite == 255)
			{
				delete[] buffer;
				virtAddr = virtAddr + 255;
				// Cộng 255 vào vị trí hiện tại
				buffer = User2System(virtAddr, 255); // Copy chuoi tu vung nho User Space sang System Space voi bo dem buffer dai 255
				lengthWrite = 0;					 
			}
		}
        DEBUG(dbgSys,"\n size = "<< size);
		kernel->machine->WriteRegister(2, size); // Tra ve so byte thuc su write duoc
		delete[] buffer;
		IncreasePC();
		return;
	}
	//  read and write
	if (kernel->fileSystem->openfile[id]->type == 0)
	{
		// Nếu số bytes viết ra lớn hơn 0
		if ((kernel->fileSystem->openfile[id]->Write(buffer, charcount)) > 0)
		{
			// So byte thuc su = NewPos - OldPos
			NewPos = kernel->fileSystem->openfile[id]->GetCurrentPos();
            DEBUG(dbgSys,"\n Write file success.");
            DEBUG(dbgSys,"\n size = "<< NewPos - OldPos);

			// tra ve bytes vào thanh ghi 2
			kernel->machine->WriteRegister(2, NewPos - OldPos);
		}
		else
		{
			kernel->machine->WriteRegister(2, -1);
		}
	}
	// // cho socket
	// else if (kernel->fileSystem->openfile[id]->type == 2 && kernel->fileSystem->openfile[id]->isConnect == true) {
	// 	tempt = kernel->fileSystem->openfile[id]->Write(buf, charcount);
	// 	if (tempt > 0) //Nếu số bytes viết ra lớn hơn 0
	// 	{
	// 	// So byte thuc su = NewPos - OldPos
	//     // tra ve bytes vào thanh ghi 2
	// 		kernel->machine->WriteRegister(2, tempt);
	// 	} else {
	// 		kernel->machine->WriteRegister(2, -1);
	// 	}
	// }
	else
	{
		kernel->machine->WriteRegister(2, -1);
	}
	delete buffer;
	IncreasePC();
	return;
}

void Seek()
{
    // input: position, id
    DEBUG(dbgSys, "\nSC_Seek Call...");
    int position = kernel->machine->ReadRegister(4);
    int id = kernel->machine->ReadRegister(5);
    // bao loi khi:
    //      goi console (id=0 || id=1)
    //      id vuot gioi han >= 20
    //      file khong ton tai
    //      goi socket

    if (id == 0 || id == 1 || id >= 20 || kernel->fileSystem->openfile[id] == NULL)
    {
        // printf("\nUnable to seek file\n");
        DEBUG(dbgSys, "\nUnable to seek file.");
        kernel->machine->WriteRegister(2, -1);
        IncreasePC();
        return;
    }

    // co the seek

    // lay kich thuoc cua file - so byte trong file
    int len = kernel->fileSystem->openfile[id]->Length();

    if (position > len)
    {
        // printf("\nInvalid position\n");
        DEBUG(dbgSys, "\nInvalid position\n");
        kernel->machine->WriteRegister(2, -1);
        IncreasePC();
        return;
    }

    // position = -1, di chuyen den cuoi file
    if (position == -1)
    {
        position = len;
    }

    kernel->fileSystem->openfile[id]->Seek(position);

    DEBUG(dbgSys,"\nposition: "<< position);
    DEBUG(dbgSys,"\nSuccess Seek\n");
    kernel->machine->WriteRegister(2, position);
    IncreasePC();
    return;
}


void Remove()
{
    // input filename
    DEBUG(dbgSys, "\nSC_Remove Call...");
    int virtAddr = kernel->machine->ReadRegister(4);
    char *filename = User2System(virtAddr, MaxFileLength + 1);
	// std::string str(filename, filename + strlen(filename));
    if (strlen(filename) == 0)
    {
        // printf("\n File name is not valid");
        DEBUG(dbgSys, "\n File name is not valid");
        kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
        delete filename;
        IncreasePC();
        return;
    }
    if (filename == NULL) // Neu khong doc duoc
    {
        // printf("\n Not enough memory in system");
        DEBUG(dbgSys, "\n Not enough memory in system");
        kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
        delete filename;
        IncreasePC();
        return;
    }

    // check file is opened
    bool isOpen = false;
    for (int i = 0; i < 20; i++)
    {
        
        DEBUG(dbgSys,"\n filename["<<i<<"] = "<<kernel->fileSystem->filename[i]);
        DEBUG(dbgSys,"\n filename = "<<filename);
        if (kernel->fileSystem->filename[i] == filename)
        {
            isOpen = true;
            break;
        }
			DEBUG(dbgSys,"i = "<< i);

    }

    // if file is opened, cannot remove
    if (isOpen == true)
    {
        DEBUG(dbgSys,"\nFile is opened, cannot remove");
        kernel->machine->WriteRegister(2, -1);
        delete[] filename;
    IncreasePC();
    return;
    }

    // file is not opened
    // call Remove method of FileSystem class
    if (kernel->fileSystem->Remove(filename))
    {
        DEBUG(dbgSys,"\nRemove success");
        kernel->machine->WriteRegister(2, 0);
    }
    else
    {
        DEBUG(dbgSys,"\nRemove unsuccess");
        kernel->machine->WriteRegister(2, -1);
    }

    delete[] filename;
    IncreasePC();
    return;
}


void ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

	switch (which)
	{
	case SyscallException:
		switch (type)
		{
		case SC_Halt:
			DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

			SysHalt();

			ASSERTNOTREACHED();
			break;

		case SC_Create:
		{
			Create();
			return;
			ASSERTNOTREACHED();
			break;
		}

		case SC_Open:
		{
			OpenfileID();
			return;
			ASSERTNOTREACHED();
			break;
		}

		case SC_Close:
		{
			Close();
			return;
			ASSERTNOTREACHED();
			break;
		}
		case SC_Read:
		{
			Read();
			return;
			ASSERTNOTREACHED();
			break;
		}
		case SC_Write:
		{
			Write();
			return;
			ASSERTNOTREACHED();
			break;
		}
		case SC_Seek:
		{
			Seek();
			return;
			ASSERTNOTREACHED();
			break;
		}

		case SC_Remove:
		{
			Remove();
			return;
			ASSERTNOTREACHED();
			break;
		}

		case SC_Add:
			DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			int result;
			result = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							/* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Add returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)result);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;

		default:
			cerr << "Unexpected system call " << type << "\n";
			break;
		}
		break;
	default:
		cerr << "Unexpected user mode exception" << (int)which << "\n";
		break;
	}
	ASSERTNOTREACHED();
}
