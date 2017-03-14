#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include <debug.h>
#include "devices/shutdown.h"
#include "lib/stdio.h"
#include "devices/input.h"

#include "threads/malloc.h" //void* calloc (size_t a, size_t b) 


struct list openfilelist;

//struct lock sysCallLock; //data lock for system calls

/* 
    adds open files to a list of open files
*/
int AddOpenFiles(int fd, struct openfiles* file_o)
{
    file_o->_fd = fd;
    list_push_back(&openfilelist, &file_o->_elem);

    return 0;
}

/*
    returns file pointed to by fd
*/
struct file* GetOpenFile(int fd)
{
    struct list_elem *currentelem; //current list element we're pointing to
    struct openfiles *of; //the file we're going to return

    currentelem = list_tail(&openfilelist);

    while((currentelem = list_prev(currentelem)) != list_head(&openfilelist))
    {
        of = list_entry(currentelem, struct openfiles, _elem);

        if(of->_fd == fd) //fd should be correct
        {
            return of->_file;
        }
    }

    return NULL;
}

/*
    Give new opened file a File Descriptor
*/
int getFileDescriptor(void)
{
    static int fd = 1; //start at 1 for STDIN/OUT

    return ++fd;
}

static void syscall_handler (struct intr_frame *);

void syscall_init (void) 
{
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
    list_init (&openfilelist);

    //lock_init(&sysCallLock);
}

/*
    Checks for a valid user pointer
*/
static uint32_t load_stack(struct intr_frame *f, int offset) //dunno what this does but I hate it
{
    bool check = f->esp + offset < 0xc0000000; //true if pointer is less than stack

    switch(check)
    {
        case 1: 
            return *((uint32_t*)(f->esp + offset));
            break;
        default:
            return -1;
            break;
    }
}

//our part 2
/*
    System Call handler.
        Arguments are given in order.
*/
static void syscall_handler (struct intr_frame *f) // UNUSED)
{
    uint32_t *esp;
    esp = f->esp;

    //printf ("System Call!\n");
    //hex_dump((int)esp, esp, 256, 1);

    printf("syscall: %d\n", *esp); //print out system call

    //int sysnum = *esp;

    switch(*esp)
    {
        case SYS_HALT: //0, works!
            halt();
            break;
        case SYS_EXIT: //1
            exit(*(esp+1));
            break;
        case SYS_EXEC: //2
            f->eax = exec(*(esp+1));
            break;
        case SYS_WAIT: //3
            f->eax = wait(*(esp+1));
            break;
         case SYS_CREATE: //4
            f->eax = create ((char*)*(esp+1), *(esp+2));
            break;
         case SYS_REMOVE: //5
            f->eax = remove ((char*)*(esp+1));
            break;
         case SYS_OPEN: //6
            f->eax = open((char*)*(esp+1));
            break;
         case SYS_FILESIZE: //7
            f->eax = filesize(*(esp+1));
            break;
         case SYS_READ: //8
            f->eax = read(*(esp+1), (void*)*(esp+2), *(esp+3));            
            break;
         case SYS_WRITE: //9 
            f->eax = write(*(esp+1), (void*)*(esp+2), *(esp+3));
            break;
         case SYS_SEEK: //10
            seek(*(esp+1), *(esp+2));
            break;
         case SYS_TELL: //11
            f->eax = tell (*(esp+1));
            break;
         case SYS_CLOSE: //12
            close(*(esp+1));
            break;
        default:
            printf("Probably an invalid pointer.\n Don't know how to handle this");
            break;
    }
}

/*Halts, powers off, the system*/
void halt (void) //0
{
    shutdown_power_off ();
}

/*    
    Terminates user program, returns exit status to Kernel
    tid_t = kernel thread, contains programs?
    pid_t = user process
*/
void exit (int status) //1
{
    struct thread *currentThread = thread_current();
    printf ("\n%s: exit(%d)\n", currentThread->name, status);
    thread_exit();
}

/*
    Runs executeable whos name is cmd_line,
    Executes a a process as a child.
*/
pid_t exec (const char *cmd_line) //2
{
    pid_t id = process_execute(cmd_line);

    //struct thread newThread =

    struct thread *t = getThreadByID(id);

    t->isChild = 1;

    return id;
}

/*
    Child process status blocking and inheritance and stuff
*/
int wait (pid_t pid) //3
{
    int success = 0;

    process_wait(pid);

    return success;
}

/*
    Create a new file on the filesystem
*/
bool create (const char *file, unsigned initial_size) //4
{
    bool success = 0;

    //bool filesys_create (const char *name, off_t initial_size) 
    success = filesys_create(file, initial_size);

    return success;
}

/*
    Remove a file from the filesystem
*/
bool remove (const char *file) //5
{
        bool success = 0;

        success = filesys_remove(file);

        return success;
}

/*
    opens a file called 'name' and returns it's FD number
    FD is stored in a seperate struct
*/
int open (const char *file) //6
{
    struct file *f;

    if(file != NULL) //null string
    {
        f = filesys_open(file);
    }

    struct openfiles fileo;

    if(f != NULL) //null struct
    {
        fileo._fd = getFileDescriptor();
        fileo._file = f;

        if(AddOpenFiles(fileo._fd, &fileo) == -1)
        {
            return -1;
        }

        return fileo._fd; //will always increment above 0 and 1 which are reserved
    }

    return -1;
}

/*
    Returns a files, fd, size
*/
int filesize (int fd) //7
{
    int size = 1;

    struct file* f;

    f = GetOpenFile(fd);

    if(f != NULL)
    {
        size = file_length(f);
        return size;
    }

    return size;
}

/*
    Reads from fd 
    into the buf 
    for the amount of size
*/
int read (int fd, void *buffer, unsigned size) //8
{
    struct file *f;

    if(fd == STDIN_FILENO) //read from keyboard
    {
        unsigned i = 0;
        char* buf = (char*) buffer;

        while((i < size))// && (*(buf+i) != '\n')) //do I need to stop at new line or what        
        {
            *(buf+i) = input_getc();
            ++i;
        }

        return size - i; //returns 0 if end of file

    }
    else if(fd > 1) //correct fd
    {
        f = GetOpenFile(fd);

        return file_read(f, buffer, size); //read from file
    }
    else
    {
        return -1; //error
    }
}

/*
    Writes to file ID'd by fd 
    returns bytes written.
    fd = 1 means write to console. (STDOUT_FILENO)
*/
int write (int fd, const void *buffer, unsigned size) //9
{
    struct file *f;

    switch(fd)
    {
        case STDOUT_FILENO:
            putbuf(buffer, size); //TODO: seperate bytes
            break;
        case STDIN_FILENO:
            return -1; //error, can't write to in
            break;
        default:

            f = GetOpenFile(fd);

            if(f != NULL)
            {
                return file_write(f, buffer, size);
            }
            else //failed to find file
            {
                return -1;
            }
            break;
    }

    return size;
}

/*
    Sets files position to position bytes from the start of the file
*/
void seek (int fd, unsigned position) //10
{
    struct file* f;

    f = GetOpenFile(fd);

    if(f != NULL)
    {
        file_seek(f, position);
    }
}

/*
    Returns the position in bytes from the start of the file
*/
unsigned tell (int fd) //11
{
    unsigned pos = 0;

    struct file* f;

    f = GetOpenFile(fd);

    if(f != NULL)
    {
        pos = (unsigned)file_tell (f);
        return pos;
    }

    return 0;
}

/*
    closes file at fd
*/
void close (int fd) //12
{
    struct list_elem *currentelem, *prevelem;

    struct openfiles *of; 

    currentelem = list_end (&openfilelist);

    while (currentelem != list_head (&openfilelist))
    {
        prevelem = list_prev (currentelem);
        of = list_entry (currentelem, struct openfiles, _elem);

        if (of->_fd == fd)
        {
            list_remove (currentelem);
            file_close (of->_file);
            free (of);
            return;
        }

        currentelem = prevelem;
    }

    return;
}
