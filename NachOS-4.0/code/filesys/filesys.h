// filesys.h
//	Data structures to represent the Nachos file system.
//
//	A file system is a set of files stored on disk, organized
//	into directories.  Operations on the file system have to
//	do with "naming" -- creating, opening, and deleting files,
//	given a textual file name.  Operations on an individual
//	"open" file (read, write, close) are to be found in the OpenFile
//	class (openfile.h).
//
//	We define two separate implementations of the file system.
//	The "STUB" version just re-defines the Nachos file system
//	operations as operations on the native UNIX file system on the machine
//	running the Nachos simulation.
//
//	The other version is a "real" file system, built on top of
//	a disk simulator.  The disk is simulated using the native UNIX
//	file system (in a file named "DISK").
//
//	In the "real" implementation, there are two key data structures used
//	in the file system.  There is a single "root" directory, listing
//	all of the files in the file system; unlike UNIX, the baseline
//	system does not provide a hierarchical directory structure.
//	In addition, there is a bitmap for allocating
//	disk sectors.  Both the root directory and the bitmap are themselves
//	stored as files in the Nachos file system -- this causes an interesting
//	bootstrap problem when the simulated disk is initialized.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef FS_H
#define FS_H

#include "copyright.h"
#include "sysdep.h"
#include "openfile.h"
// #include "debug.h"

#define MAX_FILES 20
#define CONSOLE_IN 0
#define CONSOLE_OUT 1
#define MODE_READWRITE 0
#define MODE_READ 1
#define STDIN 2
#define STDOUT 3
// #define MODE_WRITE 2
// -------note---------
typedef int OpenFileID;
// -------note---------

#ifdef FILESYS_STUB // Temporarily implement file system calls as
// calls to UNIX, until the real file system
// implementation is available
class FileSystem
{
public:
    OpenFile **openfile;
    // char *filename[20];

    //
    FileSystem(bool format)
    {
        // file descriptor table
        openfile = new OpenFile *[MAX_FILES];
        // for (int i = 0; i < 20; i++)
        // {
        //     filename[i] = new char[100];
        // }
        for (int i = 0; i < MAX_FILES; i++)
        {
            openfile[i] = NULL;
        }
        

        // tao console input va console ouput
        this->Create("stdin",0);
        this->Create("stdout",0);

        // cai dat 2 phan tu dau cho console input va output
        OpenFile *in = this->Open("stdin",2);
        openfile[CONSOLE_IN] = in; // index 0
        // openfile[CONSOLE_IN]->type = MODE_READ;

        OpenFile* out = this->Open("stdout",3);
        openfile[CONSOLE_OUT] = out; // index 1
        // openfile[CONSOLE_OUT]->type = MODE_READWRITE;
    }

    // if table is full, return -1
    // else return id of first NULL element
    int findFreeSlot()
    {
        for (int i = 2; i < MAX_FILES; i++)
        {
            if (openfile[i] == NULL)
            {
                return i;
            }
        }
        return -1;
    }

    ~FileSystem()
    {

        for (int i = 0; i < MAX_FILES; i++)
        {
            if (openfile[i] != NULL)
            {
                delete openfile[i];
            }
        }
        delete[] openfile;
    }

    bool Create(char *name)
    {
        int fileDescriptor = OpenForWrite(name);

        if (fileDescriptor == -1)
            return FALSE;

        Close(fileDescriptor);
        return TRUE;
    }

    bool Create(char *name, int initialSize) { 
		int fileDescriptor = OpenForWrite(name);

		if (fileDescriptor == -1) return FALSE;
		Close(fileDescriptor); 
		return TRUE; 
	}

    OpenFile *Open(char *name)
    {

        int fileDescriptor = OpenForReadWrite(name, FALSE);

        if (fileDescriptor == -1)
            return NULL;

        return new OpenFile(fileDescriptor,name);
    }



    // // huy
    OpenFile *Open(char *name, int type)
    {

        int fileDescriptor = OpenForReadWrite(name, FALSE);
        if (fileDescriptor == -1)
        {
            return NULL;
        }

        return new OpenFile(fileDescriptor, type,name);
    }
    // //

    bool Remove(char *name) { return Unlink(name) == 0; }
};

#else // FILESYS
class FileSystem
{
public:
    FileSystem(bool format); // Initialize the file system.
                             // Must be called *after* "synchDisk"
                             // has been initialized.
                             // If "format", there is nothing on
                             // the disk, so initialize the directory
                             // and the bitmap of free blocks.

    bool Create(char *name, int initialSize);
    // Create a file (UNIX creat)

    OpenFile *Open(char *name); // Open a file (UNIX open)

    bool Remove(char *name); // Delete a file (UNIX unlink)

    void List(); // List all the files in the file system

    void Print(); // List all the files and their contents

private:
    OpenFile *freeMapFile;   // Bit map of free disk blocks,
                             // represented as a file
    OpenFile *directoryFile; // "Root" directory -- list of
                             // file names, represented as a file
};

#endif // FILESYS

#endif // FS_H
