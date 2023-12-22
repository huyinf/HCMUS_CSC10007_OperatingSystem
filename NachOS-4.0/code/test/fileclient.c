#include "../userprog/syscall.h"
// #include <ctype.h>
#define maxLength 32

int main()
{
    // Khoi tao bien ban dau
    char buffer[100];
    char filename[maxLength], filename1[maxLength];
    int len;
    int id, id1, sizefile;

    // Khoi tao socket
    int socket1 = SocketTCP();

    // Kiem tra ket noi toi server
    Connect(socket1, "127.0.0.1", 12345);

    // Nhap file can doc noi dung (xet file a.txt)
    PrintString("\nEnter a file name: ");
    ReadString(filename, 32);

    // Mo file
    id = Open(filename, 0);

    // Kiem id nam trong file description neu co thi doc file
    if (id >= 2 && id < 20)
    {
        sizefile = Seek(-1, id);
        Seek(0, id);
        Read(buffer, sizefile, id);
    }

    // Gui noi dung trong file cho server
    len = Send(socket1, buffer, sizefile);

    // Nhan noi dung tu server
    Receive(socket1, buffer, len);

    // Nhap file can ghi vao noi dung nhan duoc tu server (xet file b.txt)
    PrintString("Enter Destination file: ");
    ReadString(filename1, 32);

    // Mo file 
    id1 = Open(filename1, 0);

    // Ghi File
    Write(buffer, sizefile, id1);

    // Dong File
    Close(id);
    Close(id1);

    // Dong Socket
    Close_Socket(socket1);
    Halt();
}