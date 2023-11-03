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
#define MaxFileLength 32
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
    /* Modify return point */
    {
        /* set previous programm counter (debugging only)*/
        kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

        /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
        kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

        /* set next programm counter for brach execution */
        kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
    }
}

// Input:
//  - User space address (int)
//  - Limit of buffer (int)
// Output:
//  - Buffer (char*)
// Purpose: Copy buffer from User memory space to System memory space
char *User2System(int virtAddr, int limit)
{
    int i; // index
    int oneChar;
    char *kernelBuf = NULL;
    kernelBuf = new char[limit + 1]; // need for terminal string
    if (kernelBuf == NULL)
        return kernelBuf;

    memset(kernelBuf, 0, limit + 1);
    for (i = 0; i < limit; i++)
    {
        kernel->machine->ReadMem(virtAddr + i, 1, &oneChar);
        kernelBuf[i] = (char)oneChar;
        if (oneChar == 0)
            break;
    }
    return kernelBuf;
}

// Input:
//  - User space address (int)
//  - Limit of buffer (int)
//  - Buffer (char[])
// Output:
//  - Number of bytes copied (int)
// Purpose: Copy buffer from System memory space to User memory space
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

void CreateFile()
{
    int virtAddr;
    char *filename;
    DEBUG(dbgFile, "\n SC_Create call ...");
    DEBUG(dbgFile, "\n Reading virtual address of filename");

    virtAddr = kernel->machine->ReadRegister(4);
    DEBUG(dbgFile, "\n Reading filename.");
    // MaxFileLength = 32
    filename = User2System(virtAddr, MaxFileLength + 1);
    // if (strlen(filename) == 0)
    // {
    //     printf("\n File name is not valid\n");
    //     DEBUG(dbgFile, "\n File name is not valid");
    //     kernel->machine->WriteRegister(2, -1);
    //     IncreasePC();
    //     return;
    //     break;
    // }
    if (filename == NULL)
    {
        printf("\n Not enough memory in system\n");
        DEBUG(dbgFile, "\n Not enough memory in system");
        // return error for user program
        kernel->machine->WriteRegister(2, -1);
        delete filename;
        IncreasePC();
        return;
    }
    DEBUG(dbgFile, "\n Finish reading filename.");
    if (!kernel->fileSystem->Create(filename))
    {
        printf("\n Error create file '%s'", filename);
        kernel->machine->WriteRegister(2, -1);
        delete filename;
        IncreasePC();
        return;
    }

    printf("\n Create file successfully");
    // return success to user program
    kernel->machine->WriteRegister(2, 1);
    delete filename;
    IncreasePC();
    return;
}

void Open()
{
    // input: filename-r4, type-r5: 0-read&write, 1-read only
    // output: fileid or -1 if fail
    // failed cases: file not existed or over 20 files

    int virtAddr;
    char *filename;
    DEBUG(dbgFile, "\n SC_Open call ...");
    DEBUG(dbgFile, "\n Reading virtual address of filename");
    virtAddr = kernel->machine->ReadRegister(4);
    DEBUG(dbgFile, "\n Reading filename.");
    filename = User2System(virtAddr, MaxFileLength + 1);

    // get type of file
    int type = kernel->machine->ReadRegister(5);

    // them ham isFull check xem mang file descriptor table
    // da day hay chua (> 20)
    // code > filesys > filesys.h > FileSystem class > isFull (public method)
    // dong thoi hoan thien class FileSystem: viet constructor/destructor.etc
    int id = kernel->fileSystem->isFull();

    if (id == -1)
    {
        printf("\n 20 files are opened");
        DEBUG(dbgFile, "\n 20 files are opened");
        kernel->machine->WriteRegister(2, -1);
        delete[] filename;
        IncreasePC();
        return;
    }

    if (filename == NULL)
    {
        printf("\n Not enough memory in system");
        DEBUG(dbgFile, "\n Not enough memory in system");
        kernel->machine->WriteRegister(2, -1); // trả về lỗi cho chương

        delete[] filename;
        IncreasePC();
        return;
    }

    // success

    // open file with type 0 or 1 only, else return error
    if (type == 0 || type == 1)
    {
        if ((kernel->fileSystem->openfile[id] = kernel->fileSystem->Open(filename, type)) != NULL)
        {
            printf("\nopen file successfully\tfile_id: %d\n", type);
            kernel->fileSystem->filename[id] = filename;
            kernel->machine->WriteRegister(2, id);
        }
        else
        {
            printf("\ncannot open file");
            kernel->machine->WriteRegister(2, -1);
        }
    }
    else
    {
        printf("cannot oepn file becacuse of wrong type");
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
        printf("\nCannot close file");
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
    return;
}

void Read()
{
    // input: buffer (char*), size(int), id(OpenFileID)
    DEBUG(dbgFile, "\n SC_Open call ...");
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
        printf("\nCannot read file because id is out of range.");
        kernel->machine->WriteRegister(2, -1);
        IncreasePC();
        return;
    }
    // check if file is existed
    if (kernel->fileSystem->openfile[id] == NULL)
    {
        printf("\nCannot read file because not exist.");
        kernel->machine->WriteRegister(2, -1);
        IncreasePC();
        return;
        // break;
    }
    // id==1 --> console stdout, this file cannot be read, return -1
    if (id == 1)
    {
        printf("\nCannot read file stdout.");
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
        // su dung ham read cua lop SynchConsole de tra ve so byte thuc su doc duoc
        int size = 0;

        for (int i = 0; i < charcount; i++)
        {
            size++;
            buffer[i] = kernel->ReadChar();
            // cai dat phuong thuc ReadChar de doc tung ki tu vao lop Kernel
            // file : code > thread > kernel.cc
            // su dung phuong thuc getChar cua lop synchConsoleIn

            // quy dinh chuoi ket thuc la \n
            // ko doc het file
            if (buffer[i] == '\n')
            {
                buffer[i + 1] = '\0';
                break;
            }
        }
        buffer[size] = '\0';

        // copy chuoi buffer tu vung nho System Space vao User Space
        // voi bo dem buffer la so byte thuc su
        System2User(virtAddr, size, buffer);
        // buffer duoc truyen vao se mang chuoi da doc

        // tra ve so byte thuc su doc duoc
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
        kernel->machine->WriteRegister(2, NewPos - OldPos);
    }
    // file content is NULL
    else
    {
        printf("\nEmpty file.");
        kernel->machine->WriteRegister(2, -1);
    }
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
        printf("\nCannot write file because id is out of range.");
        printf("id: %d\n", id);
        kernel->machine->WriteRegister(2, -1);
        IncreasePC();
        return;
    }
    // Kiem tra file co ton tai khong
    if (kernel->fileSystem->openfile[id] == NULL)
    {
        printf("\nCannot write file because file is not existed.");
        kernel->machine->WriteRegister(2, -1);
        IncreasePC();
        return;
    }
    // ghi file only read (type quy uoc la 1)
    // hoac file stdin (type quy uoc la 2) thi tra ve -1
    if (id == 0 || kernel->fileSystem->openfile[id]->type == 1)
    {
        printf("\nCannot write file stdin or read only file.");
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
            kernel->machine->WriteRegister(2, NewPos - OldPos);
            delete[] buffer;
            IncreasePC();
            return;
        }
    }

    // Xet truong hop con lai ghi file stdout
    if (id == 1)
    {
        int lengthWrite = 0;
        // dung -> vi tri chuoi bang 0
        while (buffer[lengthWrite] != 0)
        {
            kernel->putChar(buffer[lengthWrite]);
            lengthWrite++;
            // Truong hop chuoi vuot qua 255
            if (lengthWrite == 255)
            {
                delete[] buffer;
                virtAddr = virtAddr + 255;
                // Cộng 255 vào vị trí hiện tại
                buffer = User2System(virtAddr, 255); // Copy chuoi tu vung nho User Space sang System Space voi bo dem buffer dai 255
                lengthWrite = 0;                     //
            }
        }
        kernel->machine->WriteRegister(2, lengthWrite - 1); // Tra ve so byte thuc su write duoc
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
    DEBUG(dbgFile, "\nSC_Seek Call...");
    int position = kernel->machine->ReadRegister(4);
    int id = kernel->machine->ReadRegister(5);
    // bao loi khi:
    //      goi console (id=0 || id=1)
    //      id vuot gioi han >= 20
    //      file khong ton tai
    //      goi socket

    if (id == 0 || id == 1 || id >= 20 || kernel->fileSystem->openfile[id] == NULL)
    {
        printf("\nUnable to seek file\n");
        DEBUG(dbgFile, "\nUnable to seek file.");
        kernel->machine->WriteRegister(2, -1);
        IncreasePC();
        return;
    }

    // co the seek

    // lay kich thuoc cua file - so byte trong file
    int len = kernel->fileSystem->openfile[id]->Length();

    if (position > len)
    {
        printf("\nInvalid position\n");
        DEBUG(dbgFile, "\nInvalid position\n");
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
    printf("\nposition: %d", position);
    printf("\nSuccess Seek\n");
    kernel->machine->WriteRegister(2, position);
    IncreasePC();
    return;
}
void Remove()
{
    // input filename
    DEBUG(dbgFile, "\nSC_Remove Call...");
    int virtAddr = kernel->machine->ReadRegister(4);
    char *filename = User2System(virtAddr, MaxFileLength + 1);
    if (strlen(filename) == 0)
    {
        printf("\n File name is not valid");
        DEBUG(dbgFile, "\n File name is not valid");
        kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
        delete filename;
        IncreasePC();
        return;
    }
    if (filename == NULL) // Neu khong doc duoc
    {
        printf("\n Not enough memory in system");
        DEBUG(dbgFile, "\n Not enough memory in system");
        kernel->machine->WriteRegister(2, -1); // Return -1 vao thanh ghi R2
        delete filename;
        IncreasePC();
        return;
    }

    // check file is opened
    bool isOpen = false;
    for (int i = 0; i < 20; i++)
    {
        if (filename == kernel->fileSystem->filename[i])
        {
            isOpen = true;
            break;
        }
    }

    // if file is opened, cannot remove
    if (isOpen == true)
    {
        printf("\nFile is opened, cannot remove\n");
        kernel->machine->WriteRegister(2, -1);
        IncreasePC();
        delete[] filename;
        return;
    }

    // file is not opened
    // call Remove method of FileSystem class
    if (kernel->fileSystem->Remove(filename))
    {
        printf("\nRemove success\n");
        kernel->machine->WriteRegister(2, 0);
    }
    else
    {
        printf("\nRemove unsuccess\n");
        kernel->machine->WriteRegister(2, -1);
    }

    delete[] filename;
    IncreasePC();
    return;
}

void Socket()
{
    int tempt=kernel->fileSystem->isFull();
	if(tempt==-1)
	{
		printf("\n over 20 files/sockets are opened");
		kernel->machine->WriteRegister(2,-1);
		IncreasePC();
		return;
		break;
	}
	if((kernel->fileSystem->openfile[tempt]=kernel->fileSystem->socketTCP())!=NULL)
	{
		cout<<"Success open socket "<< endl;
		kernel->fileSystem->filename[tempt]= "socket";
		kernel->machine->WriteRegister(2,tempt);					
	}
	else
	{
		cout<<"Cannot open socket"<<endl;
		kernel->machine->WriteRegister(2,-1);
	}
	IncreasePC();
	return;
	break;
}

void Connect()
{
    int socketid = kernel->machine->ReadRegister(4); // Lay socketid tu thanh ghi so 5
    int virtAddr = kernel->machine->ReadRegister(5);  // Lay dia chi cua tham so ip tu thanh ghi so 4
	int port = kernel->machine->ReadRegister(6);        // Lay port tu thanh ghi so 6
	char* ip;
	if (socketid < 2 || socketid >=20 )
	{
	    printf("\nKhong the connect vi id nam ngoai bang mo ta file/socket.");
		kernel->machine->WriteRegister(2, -1); // -1: Loi
		IncreasePC();
		return;
		break;
	}
	if (kernel->fileSystem->openfile[socketid] == NULL)
	{
		printf("\nKhong the connect vi socket nay khong ton tai.");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
		break;
	}
	ip = User2System(virtAddr, MAX_LENGTH_IP); 
	if(kernel->fileSystem->openfile[socketid]->Connect(ip, port)<0){
		cout << ip;
		kernel->machine->WriteRegister(2, -1);
	} else {
		kernel->machine->WriteRegister(2, 0);
	}
	delete [] ip ;
	IncreasePC();
	return;
	break;
}

void Send()
{
    int virtAddr = kernel->machine->ReadRegister(4);  // Lay dia chi cua tham so buffer tu thanh ghi so 4
	int charcount = kernel->machine->ReadRegister(5); // Lay charcount tu thanh ghi so 5
	int id = kernel->machine->ReadRegister(6);        // Lay id cua file tu thanh ghi so 6
	int tempt;
	char *buf;
	if (id < 0 || id >= 20)
	{
	    printf("\nKhong the send vi id nam ngoai bang mo ta file.");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
	}
	if (kernel->fileSystem->openfile[id] == NULL)
	{
		printf("\nKhong the send vi socket nay khong ton tai.");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
	}
	buf = User2System(virtAddr, charcount);// Copy chuoi tu vung nho User Space sang System Space voi bo dem buffer dai charcount
	if (kernel->fileSystem->openfile[id]->type == 2 && kernel->fileSystem->openfile[id]->isConnect == true) {
		tempt = kernel->fileSystem->openfile[id]->Write(buf, charcount);
		if (tempt > 0) //Nếu số bytes viết ra lớn hơn 0
		{
		// So byte thuc su = NewPos - OldPos
			kernel->machine->WriteRegister(2, tempt); // Viết bytes vào thanh ghi 2
		} else {
			kernel->machine->WriteRegister(2, -1);
		}
	} else {
		kernel->machine->WriteRegister(2, -1);
	}
	delete buf;
	IncreasePC();
	return;

}

void Receive()
{
    int virtAddr = kernel->machine->ReadRegister(4);  // Lay dia chi cua tham so buffer tu thanh ghi so 4
	int charcount = kernel->machine->ReadRegister(5); // Lay charcount tu thanh ghi so 5
	int id = kernel->machine->ReadRegister(6);        // Lay id cua file tu thanh ghi so 6
	int OldPos;
	int NewPos;
	char *buf;
	int tempt;
	// Thầy chỉ cho 20 file 
	if (id < 2 || id >= 20 )
	{
		printf("\nKhong the receive vi id nam ngoai bang mo ta socket.");
		kernel->machine->WriteRegister(2, -1); // -1: Loi
		IncreasePC();
		return;
		break;
	}
	// Kiem tra file co ton tai khong
	if (kernel->fileSystem->openfile[id] == NULL)
	{
		printf("\nKhong the receive vi socket nay khong ton tai.");
		kernel->machine->WriteRegister(2, -1);
		IncreasePC();
		return;
		break;
	}
    buf = User2System(virtAddr, charcount); 
	if (kernel->fileSystem->openfile[id]->type == 2 && kernel->fileSystem->openfile[id]->isConnect == true) { // socket
		tempt = kernel->fileSystem->openfile[id]->Read(buf, charcount);
		if (tempt >0 ) {
			System2User(virtAddr, tempt, buf); //bytes=newpos-oldpos
			kernel->machine->WriteRegister(2, tempt);
		} else {
			kernel->machine->WriteRegister(2, -1);
		}
	} else {
		kernel->machine->WriteRegister(2, -1);
	}
	delete buf;
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
            DEBUG(dbgSys, "Shutdown, initiated by user program.");

            SysHalt();

            ASSERTNOTREACHED();
            break;
        case SC_Create:
        {
            CreateFile();
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
            break;
        }
        case SC_Socket:
        {
            Socket();
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
        // cach lam doc lap:
        // tao file descriptor table rieng biet cho phan 1 va phan 2
        // thao tac binh thuong

        // cach rut gon: --> cho phan nang cao
        // luu file descriptor table nhu mot thuoc tinh public cua FileSystem
        // thao tac voi FileSystem Object cho tung phan

        // case SC_Socket:
        // {
        //     int id = kernel->fileSystem->isFull();
        //     if (id == -1)
        //     {
        //         printf("\n 20 sockets/files are opened");
        //         kernel->machine->WriteRegister(2, -1);
        //         IncreasePC();
        //         return;
        //         break;
        //     }
        // }
        case SC_Add:
            DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

            /* Process SysAdd Systemcall*/
            int result;
            result = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
                            /* int op2 */ (int)kernel->machine->ReadRegister(5));

            DEBUG(dbgSys, "Add returning with " << result << "\n");
            /* Prepare Result */
            kernel->machine->WriteRegister(2, (int)result);

            IncreasePC();
            return;

            // ASSERTNOTREACHED();

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
    // ASSERTNOTREACHED();
}
