#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/user/syscall.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "lib/kernel/list.h"

#define PHYS_BASE 0xc0000000

/*  
    struct to hold open files
    typedef struct openfiles of
*/
struct openfile
{
    int _fd;
    struct file* _file;
    struct list_elem _elem;
};


//struct lock sysCallLock; //data lock for system calls

int AddOpenFiles(int fd, struct openfile *file_o);
struct file* GetOpenFile(int fd);
int getFileDescriptor(void);

void syscall_init (void);

/*System calls*/
void halt (void);//done
void exit (int status); //done

pid_t exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);

int open (const char *file); //done

int filesize (int fd);

int read (int fd, void *buffer, unsigned size); //done, maybe need to check that termination
int write (int fd, const void *buffer, unsigned size); //done

void seek (int fd, unsigned position);
unsigned tell (int fd);

void close (int fd); //done


#endif /* userprog/syscall.h */
