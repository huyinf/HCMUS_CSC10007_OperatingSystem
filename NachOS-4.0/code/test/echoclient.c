#include "../userprog/syscall.h"
int main()
{
    // Tao 4 socket ket noi den server
    int socketid1 = SocketTCP();
    int socketid2 = SocketTCP();
    int socketid3 = SocketTCP();
    int socketid4 = SocketTCP();

    // luu tru do dai noi dung
    int len1, len2, len3, len4;
    
    // luu tru noi dung
    char buffer1[100],buffer2[100],buffer3[100],buffer4[100];

    // ket noi server
    Connect(socketid1, "127.0.0.1", 12345);
    Connect(socketid2, "127.0.0.1", 12345);
    Connect(socketid3, "127.0.0.1", 12345);
    Connect(socketid4, "127.0.0.1", 12345);

    // gui noi dung cho server
    len1 = Send(socketid1, "Hello world 1", 100);
    len2 = Send(socketid2, "Hello world 2", 100);
    len3 = Send(socketid3, "Hello world 3", 100);
    len4 = Send(socketid4, "Hello world 4", 100);
    
    // Nhan noi dung tu server
    Receive(socketid1, buffer1, len1);
    Receive(socketid2, buffer2, len2);
    Receive(socketid3, buffer3, len3);
    Receive(socketid4, buffer4, len4);

    // In noi dung nhan duoc tu server
    PrintString(buffer1);
    PrintString(buffer2);
    PrintString(buffer3);
    PrintString(buffer4);

    // Close socket    
    Close_Socket(socketid1);
    Close_Socket(socketid2);
    Close_Socket(socketid3);
    Close_Socket(socketid4);

    Halt();
  /* not reached */
}
