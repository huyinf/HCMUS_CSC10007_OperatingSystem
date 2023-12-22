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
#include "synchconsole.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring> // For strlen
#include "openfile.h"

#define MaxFileLength 32
#define MAX_CHARACTERS 100
#define MAX_OPEN_FILE_NAME 25
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
#define MAX_FDS 20
#define MAX_LENGTH_STRING 32
#define MAX_OPEN_FILE_NAME 25
#define MAX_CONTENT 100

#define ONLY_READ 1
#define READ_WRITE 0

// Gan co cho socket trong struct .flag
#define SOCKET 10
#define OPENFILE 20

int fds = 0;
int i = 0;

struct table
{
	int socket;
	OpenFile *Fileid;
	int typeFile;
	int flag;
};
// Khai cau truc file description voi 20 phan tu: socket or file
table list[20];

void createConsoleInOut()
{
	// console input

	if (list[0].Fileid == NULL)
	{
		kernel->fileSystem->Create("stdin");
		list[0].Fileid = kernel->fileSystem->Open("stdin");
		list[0].typeFile = 1; // read only
		list[0].flag = OPENFILE;
	}
	// console output
	if (list[1].Fileid == NULL)
	{
		kernel->fileSystem->Create("stdout");
		list[1].Fileid = kernel->fileSystem->Open("stdout");
		list[1].typeFile = 0; // read and write
		list[1].flag = OPENFILE;
	}
}

int posFree = 2;

// Tim vi tri trong tiep theo trong file description
void countPosFree()
{
	for (int i = 0; i < 20; i++)
	{
		if (list[posFree].socket || list[posFree].Fileid)
		{
			posFree++;
		}
	}
}

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

void SysPrintChar(char character)
{
	kernel->synchConsoleOut->PutChar(character);
}

void SysPrintString()
{
	int idbuffer = kernel->machine->ReadRegister(4);

	// Allocate a buffer for content
	char *content = User2System(idbuffer, MAX_CHARACTERS);

	// Print the string
	for (int i = 0; i < strlen(content); i++)
	{
		kernel->synchConsoleOut->PutChar(content[i]);
	}
	kernel->synchConsoleOut->PutChar('\n');

	// Free the allocated memory
	delete[] content;
	return;
}

void Create()
{
	int virtAddr;	// tao dia chi
	char *filename; // luu tru string
	// DEBUG('u', "\n SC_CreateFile call ...");
	// DEBUG('u', "\n Reading virtual address of filename");

	virtAddr = kernel->machine->ReadRegister(4); // Doc dia chi cua file tu thanh ghi R4
	// DEBUG('u', "\n Reading filename.");

	filename = User2System(virtAddr, MaxFileLength + 1); // copy data tu user -> kernel de xu ly
	// if (strlen(filename) == 0)
	// {
	// 	DEBUG('u', "\n File name is not valid");
	// 	kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
	// 	IncreasePC();
	// 	return;
	// }
	// if (filename == NULL) // Neu khong doc duoc
	// {
	// 	// printf("\n Not enough memory in system");
	// 	DEBUG('a', "\n Not enough memory in system");
	// 	kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
	// 	delete[] filename;
	// 	IncreasePC();
	// 	return;
	// }
	// DEBUG('u', "\n Finish reading filename.");

	if (!kernel->fileSystem->Create(filename)) // Tao file bang ham Create cua fileSystem, tra ve ket qua
	{
		// Tao file that bai
		// printf("\n Error create file '%s'", filename);
		DEBUG(dbgSys, "\nCreate file: failed!");
		kernel->machine->WriteRegister(2, -1);
		// IncreasePC();
		// return;
	}
	else
	{
		DEBUG(dbgSys, "\nCreate file: successfully!");
		kernel->machine->WriteRegister(2, 0);
	}
	// Tao file thanh cong
	// cout << "Create file success" << endl;
	delete[] filename;
	// return;
}

void Open()
{
	// Input: filename, type (0 for read and write and 1 for read file)
	// Output: failed or -1 for fail
	// case for fail: over 20 files - cannot find file name (not exist)

	int virtAddr = kernel->machine->ReadRegister(4); // Doc thanh ghi 4
	// char *filename;
	int type = kernel->machine->ReadRegister(5); // Doc thanh ghi 5

	char *filename = User2System(virtAddr, MaxFileLength + 1);
	// int tempt = kernel->fileSystem->isFull();

	list[posFree].Fileid = kernel->fileSystem->Open(filename);
	list[posFree].typeFile = type;
	list[posFree].flag = OPENFILE;

	// DEBUG(dbgSys, "\nidid: " << tempt);
	// Check over 20 files
	// if (tempt == -1)
	// {
	// 	DEBUG(dbgSys, "\n over 20 files are opened")
	// 	kernel->machine->WriteRegister(2, -1);
	// 	delete[] filename;
	// 	IncreasePC();
	// 	return;
	// }
	// // file name không nhận
	// if (strlen(filename) == 0)
	// {
	// 	// printf("\n File name is not valid");
	// 	DEBUG(dbgSys, "\n File name is not valid");
	// 	kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
	// 	IncreasePC();
	// 	return;
	// }
	// if (filename == NULL) // Neu khong doc duoc
	// {
	// 	// printf("\n Not enough memory in system");
	// 	DEBUG(dbgSys, "\n Not enough memory in system");
	// 	kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
	// 	delete[] filename;
	// 	return;
	// }

	// success
	// only open file when type = 1 or type = 0
	// if (type == 1 || type == 0)
	// {
	// 	kernel->fileSystem->openfile[tempt] = kernel->fileSystem->Open(filename, type);
	// 	if ((kernel->fileSystem->openfile[tempt]) != NULL)
	// 	{
	// 		// cout << "Success open file"
	// 		// 	 << " file id: " << tempt << endl;
	// 		DEBUG(dbgSys, "\n Success open file ");
	// 		DEBUG(dbgSys, "\n opened filename: " << filename);
	// 		DEBUG(dbgSys, "\n File id: " << tempt);

	// 		// kernel->fileSystem->filename[tempt] = filename;
	// 		kernel->machine->WriteRegister(2, tempt); // index -1 vi after open thì nó đã cộng một vị trí index
	// 												  // cout<<kernel->fileSystem->openfile[kernel->fileSystem->index-1]->type;
	// 	}
	// 	else
	// 	{
	// 		// cout << "Cannot open file" << endl;
	// 		DEBUG(dbgSys, "\nCannot open file")
	// 		kernel->machine->WriteRegister(2, -1);
	// 	}
	// }
	// else
	// {
	// 	cout << "Cannot open file because wrong type" << endl;
	// 	kernel->machine->WriteRegister(2, -1);
	// }
	// delete[] filename;
	// return;

	if (list[posFree].Fileid && posFree < 20)
	{
		DEBUG(dbgSys, "\nOpen file: succeeded");
		kernel->machine->WriteRegister(2, posFree);
		countPosFree();
	}
	else
	{
		DEBUG(dbgSys, "\nOpen file: failed!");
		kernel->machine->WriteRegister(2, -1);
	}
}

void Close()
{
	int id = kernel->machine->ReadRegister(4);
	// neu id < 2 (0,1 danh cho console input,output)
	// hoac id >= 20 (over)
	// -> bao loi, return -1
	DEBUG(dbgSys, "\nid file: " << id);
	if (id < 2 || id >= 20)
	{
		printf("\nInvalid file id\n");
		kernel->machine->WriteRegister(2, -1);
		// IncreasePC();
		// return;
	}
	else
	{
		if (list[id].flag == OPENFILE)
		{
			if (list[id].Fileid != NULL)
			{
				DEBUG(dbgSys, "\nClose file: succeeded");
				kernel->machine->WriteRegister(2, 0);
				delete list[id].Fileid;
				list[id].Fileid = NULL;
			}
			else
			{
				DEBUG(dbgSys, "\nClose file error: not exist");
				kernel->machine->WriteRegister(2, -1);
			}
		}
		else
		{
			DEBUG(dbgSys, "\nCannot close: not a file");
			kernel->machine->WriteRegister(2, -1);
		}
	}

	// return 0 if success
	// kernel->machine->WriteRegister(2, 0);
	// kernel->fileSystem->filename[id] = "";
	// delete kernel->fileSystem->openfile[id];
	// kernel->fileSystem->openfile[id] = NULL;
	// // dung ham printf gap loi kieu du lieu string nen da them c_strc()
	// printf("%s\n", kernel->fileSystem->filename[id].c_str());
	// printf("\nClose file successfully");
	// return;
}

void Read()
{
	DEBUG(dbgSys, "\n Read ......... ");
	int idbuffer = kernel->machine->ReadRegister(4);
	int charcount = kernel->machine->ReadRegister(5);
	int ID = kernel->machine->ReadRegister(6);
	if (list[ID].flag == OPENFILE)
	{

		// file ton tai va khong la console output
		if (list[ID].Fileid != NULL && ID != 1)
		{
			// file binh thuong - read only va read and write
			if (ID != 0)
			{
		char *buffer = User2System(idbuffer, charcount);

				int result = list[ID].Fileid->Read(buffer, charcount);
				DEBUG(dbgSys, "\n SIZE = " << charcount);
				DEBUG(dbgSys, "\n Read file successful");
				kernel->machine->WriteRegister(2, result);
				DEBUG(dbgSys, "\n res = " << result);

				// writeToMem(buffer, MAX_OPEN_FILE_NAME, idbuffer);
				System2User(idbuffer, MAX_OPEN_FILE_NAME, buffer);
			}
			// console input
			else
			{
				DEBUG(dbgSys, "\nread stdin file");
				// su dung ham read cua lop SynchConsole de tra ve so byte thuc su doc duoc
				int size = 0;

				char *newBuff = new char[MAX_CHARACTERS];
				memset(newBuff, 0, MAX_CHARACTERS);

				char c = kernel->ReadChar();
				// if character is \n, newline, enter
				int cnt = 0;
				while (c != (char)10)
				{
					size++;
					newBuff[cnt] = c;

					if (size > MAX_CHARACTERS)
					{
						DEBUG(dbgSys, "\n Overflow");
						break;
					}
					cnt++;
					c = kernel->ReadChar();
				}
				DEBUG(dbgSys, "\nsize = " << size);

				// buffer[size] = '\0';

				// copy chuoi buffer tu vung nho System Space vao User Space
				// voi bo dem buffer la so byte thuc su
				System2User(idbuffer, size, newBuff);
				// buffer duoc truyen vao se mang chuoi da doc

				// tra ve so byte thuc su doc duoc
				DEBUG('u', "\n Size: " << size);
				kernel->machine->WriteRegister(2, size);

				// delete[] buffer;
				// IncreasePC();
				// return;
			}
		}
		else
		{
			DEBUG(dbgSys, "\n Read file not successful");
			kernel->machine->WriteRegister(2, -1);
		}
	}
	else
	{
		// static char contentSend[MAX_CONTENT];
		// bzero(contentSend, MAX_CONTENT);
		// readFromMem(contentSend, MAX_CONTENT, idbuffer);
		char *contentSend = User2System(idbuffer, MAX_CONTENT);

		int shortRetval = -1;
		struct timeval tv;
		tv.tv_sec = 20; /* 20 Secs Timeout */
		tv.tv_usec = 0;
		if (setsockopt(list[ID].socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0)
		{
			kernel->machine->WriteRegister(2, -1);
		}
		shortRetval = send(list[ID].socket, contentSend, charcount, 0);
		kernel->machine->WriteRegister(2, shortRetval);
	}
}

void Write()
{
	int idbuffer = kernel->machine->ReadRegister(4);
	int charcount = kernel->machine->ReadRegister(5);
	int ID = kernel->machine->ReadRegister(6);

	if (list[ID].flag == OPENFILE)
	{
		// char *buffer = new char[MAX_OPEN_FILE_NAME];
		// bzero(buffer, MAX_OPEN_FILE_NAME);
		// readFromMem(buffer, MAX_OPEN_FILE_NAME, idbuffer);
		char *buffer = User2System(idbuffer, MAX_OPEN_FILE_NAME);

		// ghi file da ton tai, khong phai console input, khong phai file only_read
		if (list[ID].Fileid != NULL && ID != 0 && list[ID].typeFile != ONLY_READ)
		{
			// file read and write
			if (ID != 1)
			{
				DEBUG(dbgSys, "\n Write successful");
				int result = list[ID].Fileid->Write(buffer, charcount);
				DEBUG(dbgSys, "\n res: " << result);
				kernel->machine->WriteRegister(2, result);
			}
			else
			{
				DEBUG(dbgSys, "\n Write file id = 1.");

				int size = 0;
				int lengthWrite = 0;
				// dung -> vi tri chuoi bang 0
				if(buffer == NULL){
					DEBUG(dbgSys,"\n null");
				}
				DEBUG(dbgSys,"\n buf " << buffer[lengthWrite]);

				while (buffer[lengthWrite] != NULL)
				{
					kernel->putChar(buffer[lengthWrite]);
					size++;
					lengthWrite++;
					// Truong hop chuoi vuot qua 255
					if (lengthWrite == 255)
					{
						delete[] buffer;
						idbuffer = idbuffer + 255;
						// Cộng 255 vào vị trí hiện tại
						buffer = User2System(idbuffer, 255); // Copy chuoi tu vung nho User Space sang System Space voi bo dem buffer dai 255
						lengthWrite = 0;
					}
				}
				DEBUG(dbgSys, "\n size = " << size);
				kernel->machine->WriteRegister(2, size); // Tra ve so byte thuc su write duoc
														 // delete[] buffer;
														 // IncreasePC();
														 // return;
			}
		}
		else
		{
			DEBUG(dbgSys, "\n Write not successful 1");
			kernel->machine->WriteRegister(2, -1);
		}
	}
	else
	{
		// char *contentSend = new char[MAX_CONTENT];
		// bzero(contentSend, MAX_CONTENT);
		// readFromMem(contentSend, MAX_CONTENT, idbuffer);
		char *contentSend = User2System(idbuffer, MAX_CONTENT);

		int shortRetval = -1;
		struct timeval tv;
		tv.tv_sec = 20; /* 20 Secs Timeout */
		tv.tv_usec = 0;
		if (setsockopt(list[ID].socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)
		{
			DEBUG(dbgSys, "\n Write not successful");
			kernel->machine->WriteRegister(2, -1);
			return;
		}

		shortRetval = recv(list[ID].socket, contentSend, charcount, 0);

		// writeToMem(contentSend, MAX_CONTENT, idbuffer);
		System2User(idbuffer, MAX_CONTENT, contentSend);
		kernel->machine->WriteRegister(2, shortRetval);
	}
}

void Seek()
{
	int offset = kernel->machine->ReadRegister(4);
	int openfileId = kernel->machine->ReadRegister(5);
	if (list[openfileId].Fileid != 0 && list[openfileId].typeFile == READ_WRITE)
	{

		int result = list[openfileId].Fileid->getCurOffset();
		if (offset == result)
		{
			kernel->machine->WriteRegister(2, result);
		}
		else
		{
			kernel->machine->WriteRegister(2, -1);
		}
	}
}

void Remove()
{
	DEBUG(dbgSys, "Create a file.");
	char* fileName;
	int virAddr = kernel->machine->ReadRegister(4);
	fileName = User2System(virAddr, MAX_OPEN_FILE_NAME);
	if (fileName && kernel->fileSystem->Remove(fileName))
		kernel->machine->WriteRegister(2, 0);
	else
		kernel->machine->WriteRegister(2, -1);
}

int SocketTCP()
{
	// Kiem tra vi tri trong >= 20 phan tu return -1
	if (posFree >= MAX_FDS)
	{
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return -1;
	}

	// Tra ve socket id
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		DEBUG(dbgSys, "\n Error");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();

		return -1;
	}

	DEBUG(dbgSys, "\n Socket Successfully");
	DEBUG(dbgSys, "\n ID Socket: " << fd << "\n ID fds: " << posFree);

	list[posFree].socket = fd;	 // gan socket id
	list[posFree].flag = SOCKET; // gan co

	kernel->machine->WriteRegister(2, posFree);
	countPosFree();

	return fd;
}

int Connect()
{
	int socketid = kernel->machine->ReadRegister(4);
	int ip = kernel->machine->ReadRegister(5);
	int port = kernel->machine->ReadRegister(6);
	char *Ip = new char[100];
	Ip = User2System(ip, 100);

	int iRetval = -1;
	struct sockaddr_in remote = {0};
	remote.sin_addr.s_addr = inet_addr(Ip);
	remote.sin_family = AF_INET;
	remote.sin_port = htons(port);
	iRetval = connect(list[socketid].socket, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));

	if (iRetval < 0)
	{
		DEBUG(dbgSys, "\nConnection Failed");
	}
	else
	{
		DEBUG(dbgSys, "\nConnection Successfull");
	}
	DEBUG(dbgSys, "\n IRetval: " << iRetval);
	kernel->machine->WriteRegister(2, iRetval);
	return iRetval;
}

void Send()
{
	int socketid = kernel->machine->ReadRegister(4);
	int addContent = kernel->machine->ReadRegister(5);
	int sizeContent = kernel->machine->ReadRegister(6);

	char *contentSend = new char[100];
	contentSend = User2System(addContent, 100);

	int shortRetval = -1;
	struct timeval tv;
	tv.tv_sec = 20; /* 20 Secs Timeout */
	tv.tv_usec = 0;
	if (setsockopt(list[socketid].socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0)
	{
		kernel->machine->WriteRegister(2, -1);
	}
	shortRetval = send(list[socketid].socket, contentSend, sizeContent, 0);
	kernel->machine->WriteRegister(2, shortRetval);
}

void Receive()
{
	// doc thanh ghi
	int socketid = kernel->machine->ReadRegister(4);
	int addContent = kernel->machine->ReadRegister(5);
	int sizeContent = kernel->machine->ReadRegister(6);

	char *contentSend = new char[100];
	contentSend = User2System(addContent, 100); // copy noi dung xuong kernel

	int shortRetval = -1;

	struct timeval tv;
	tv.tv_sec = 20; /* 20 Secs Timeout */
	tv.tv_usec = 0;
	if (setsockopt(list[socketid].socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)
	{
		kernel->machine->WriteRegister(2, -1);
	}
	shortRetval = recv(list[socketid].socket, contentSend, sizeContent, 0);
	/////////////////////////////////////////////
	System2User(addContent, 100, contentSend); // copy noi dung tu kernel

	kernel->machine->WriteRegister(2, shortRetval);
	DEBUG(dbgSys, "\nshortRetval: " << shortRetval);
}

void Close_Socket()
{
	// DEBUG(dbgSys, "\n aaaa  ");

	// Lay id cua thanh ghi so 4
	int id = kernel->machine->ReadRegister(4);

	int iRetval = 0;

	iRetval = close(list[id].socket); // goi ham close trong thu vien de dong file
	if (iRetval >= 0)				  // close thanh cong
	{
		kernel->machine->WriteRegister(2, 0);
		DEBUG(dbgSys, "\n Close Socket Successfully.  " << id);
	}
	else
	{
		DEBUG(dbgSys, "\n Close Socket not Successfully.");
		kernel->machine->WriteRegister(2, -1);
	}
	return;
}

void ReadString()
{
	// input char[] voi int length
	// output :None
	int virAddr; // khai bao dia chi nhan tu thanh ghi
	int length;
	int inputLength;
	char *strName;
	char c;

	virAddr = kernel->machine->ReadRegister(4); // lay dia chi tu thanh ghi (char buffer[] o user space)
	length = kernel->machine->ReadRegister(5);	// lay dia chi tu thanh
	strName = new char[length];					// day se la bien buffer duoc tra ve cho nguoi dung
	inputLength = 0;
	while ((c = kernel->ReadChar()) != '\n')
	{
		strName[inputLength] = c;
		inputLength++;
	}
	strName[inputLength] = '\0';

	int numBytes = System2User(virAddr, inputLength, strName); // chuyen bo nho qua user
	if (inputLength > length)
	{
		DEBUG(dbgSys, "\nChuoi nhap co do dai qua lon so voi quy dinh mat roi\n");
		IncreasePC();
		return;
	}
	if (numBytes == 0)
	{
		DEBUG(dbgSys, "Chuoi rong\n");
	}
	else if (numBytes > MAX_LENGTH_STRING)
	{
		DEBUG(dbgSys, "\nChuoi qua lon so vs he thong");
		IncreasePC();
		return;
	}

	IncreasePC();
	return;
}
void ExceptionHandler(ExceptionType which)
{
	createConsoleInOut();
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
			break;
		}

		case SC_Open:
		{
			Open();
			break;
		}

		case SC_Close:
		{
			Close();
			break;
		}
		case SC_Read:
		{
			Read();
			break;
		}
		case SC_Write:
		{
			Write();
			break;
		}
		case SC_Seek:
		{
			Seek();
			break;
		}

		case SC_Remove:
		{
			Remove();
			break;
		}

		case SC_SocketTCP:
		{
			SocketTCP();
			break;
		}

		case SC_Connect:
		{
			Connect();
			break;
		}

		case SC_Send:
		{
			Send();
			break;
		}

		case SC_Receive:
		{
			Receive();
			break;
		}
		case SC_Close_Socket:
		{
			Close_Socket();
			break;
		}
		case SC_PrintString:
		{
			SysPrintString();
			break;
		}
		case SC_PrintChar:
		{
			char character = (char)kernel->machine->ReadRegister(4);
			SysPrintChar(character);
			break;
		}

		case SC_ReadString:
		{
			ReadString();
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
	IncreasePC();
	return;
	ASSERTNOTREACHED();
}
