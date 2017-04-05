#include "userprog/syscall.h"

#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include <debug.h>
#include "devices/shutdown.h"
#include "lib/stdio.h"
#include "devices/input.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include <string.h>
#include "process.h"
#include "threads/malloc.h"

struct list openfilelist;

struct lock FSLock; //data lock for system calls

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

    lock_init(&FSLock);
}

/*
    Checks for a valid user pointer
*/
static uint32_t load_stack(struct intr_frame *f, int offset) //dunno what this does but I hate it
{
    bool check = f->esp + offset < PHYS_BASE; //true if pointer is less than stack, so less than PHYSBASE

    if(check == true)
    {
        return f->esp + offset;
    }
    else
    {
        return PHYS_BASE;
    }
}

//our part 2
/*
    System Call handler.
        Arguments are given in order. on the stack
*/
static void syscall_handler (struct intr_frame *f) // UNUSED)
{
    uint32_t *esp = load_stack(f, 0), //syscall
    arg1 = load_stack(f, 1), //syscall argguments
    arg2 = load_stack(f, 2),
    arg3 = load_stack(f, 3);

    bool broke = 0;

    //esp = load_stack(f, 0); //get sys call

    //printf("syscall: %d\n", *esp); //print out system call

    if(esp == PHYS_BASE)
    {   
        printf("INVALID USER POINTER!");
        exit(-1);
    }

    switch(*esp)
    {
        case SYS_HALT: //0
            halt();
            break;
        case SYS_EXIT: //1

            if(arg1 != PHYS_BASE)
                exit(*(esp+1));
            else
                broke = 1;
            
            break;
        case SYS_EXEC: //2
            
            if(arg1 != PHYS_BASE)
                f->eax = exec((char*)*(esp+1));
            else
                broke = 1;
            
            break;
        case SYS_WAIT: //3

            if(arg1 != PHYS_BASE)
                f->eax = wait(*(esp+1));
            else
                broke = 1;
            
            break;
         case SYS_CREATE: //4    

            if((arg1 != PHYS_BASE) && (arg2 != PHYS_BASE))
                f->eax = create ((char*)*(esp+1), *(esp+2));
            else
                broke = 1;
                        
            break;
         case SYS_REMOVE: //5

            if(arg1 != PHYS_BASE)
                f->eax = remove ((char*)*(esp+1));
            else
                broke = 1;

            break;
         case SYS_OPEN: //6

            if(arg1 != PHYS_BASE)
                f->eax = open((char*)*(esp+1));
            else
                broke = 1;

            break;
         case SYS_FILESIZE: //7
            
            if(arg1 != PHYS_BASE)            
                f->eax = filesize(*(esp+1));
            else
                broke = 1;

            break;
         case SYS_READ: //8
            
            if((arg1 != PHYS_BASE) && (arg2 != PHYS_BASE) && (arg3 != PHYS_BASE))
                f->eax = read(*(esp+1), (void*)*(esp+2), *(esp+3));
            else
                broke = 1;

            break;
         case SYS_WRITE: //9 

            if((arg1 != PHYS_BASE) && (arg2 != PHYS_BASE) && (arg3 != PHYS_BASE))
                f->eax = write(*(esp+1), (void*)*(esp+2), *(esp+3));
            else
                broke = 1;

            break;
         case SYS_SEEK: //10

            if((arg1 != PHYS_BASE) && (arg2 != PHYS_BASE))
                seek(*(esp+1), *(esp+2));
            else
                broke = 1;

            break;
         case SYS_TELL: //11
            
            if(arg1 != PHYS_BASE)
                f->eax = tell (*(esp+1));
            else
                broke = 1;

            break;
         case SYS_CLOSE: //12

            if(arg1 != PHYS_BASE)
                close(*(esp+1));
            else
                broke = 1;

            break;
        default:

            printf("\nINVALID SYSCALL! %d ", (int)*esp);
            exit(-1);
            break;
    }

    if(broke == 1)
    {
        printf("\nUSER POINTER ERROR!\n %d", (int)*esp);

        if(lock_held_by_current_thread(&FSLock))
        {
            lock_release(&FSLock);
        }

        exit(-1);
    }

    return;
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
    char* progName;
    char* temp;
    char* svp;

    struct thread *currentThread = thread_current();

    temp = palloc_get_page(1);

    strlcpy (temp, currentThread->name, sizeof(temp)+1);

    progName = strtok_r(temp, " ", &svp);

    printf ("%s: exit(%d)\n", progName, status);

    palloc_free_page (temp);

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
    lock_acquire(&FSLock);

    success = filesys_create(file, initial_size);

    lock_release(&FSLock);

    return success;
}

/*
    Remove a file from the filesystem
*/
bool remove (const char *file) //5
{
    bool success = 0;

    lock_acquire(&FSLock);

    success = filesys_remove(file);

    lock_release(&FSLock);

    return success;
}

/*
    opens a file called 'name' and returns it's FD number
    FD is stored in a seperate struct
*/
int open (const char *file) //6
{
    struct file *f;

    lock_acquire(&FSLock);

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

        lock_release(&FSLock);
        return fileo._fd; //will always increment above 0 and 1 which are reserved
    }

    lock_release(&FSLock);
    return -1;
}

/*
    Returns a files, fd, size
*/
int filesize (int fd) //7
{
    int size = 1;

    struct file* f;

    lock_acquire(&FSLock);

    f = GetOpenFile(fd);

    if(f != NULL)
    {
        size = file_length(f);
        lock_release(&FSLock);
        return size;
    }

    lock_release(&FSLock);
    return size;
}

/*
    Reads from the fd 
    into the buffer pointer
    for size amount
*/
int read (int fd, void *buffer, unsigned size) //8
{
    struct file *f;

    lock_acquire(&FSLock);
    if(fd == STDIN_FILENO) //read from keyboard
    {
        unsigned i = 0;
        char* buf = (char*) buffer;

        while((i < size))// && (*(buf+i) != '\n')) //do I need to stop at new line or what        
        {
            *(buf+i) = input_getc();
            ++i;
        }

        lock_release(&FSLock);
        return size - i; //returns 0 if end of file

    }
    else if(fd > 1) //correct fd
    {
        f = GetOpenFile(fd);

        lock_release(&FSLock);
        return file_read(f, buffer, size); //read from file
    }
    else
    {
        lock_release(&FSLock);
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

    lock_acquire(&FSLock);

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
                lock_release(&FSLock);
                return file_write(f, buffer, size);
            }
            else //failed to find file
            {
                lock_release(&FSLock);
                return -1;
            }
            break;
    }

    lock_release(&FSLock);
    return size;
}

/*
    Sets files position to position bytes from the start of the file
*/
void seek (int fd, unsigned position) //10
{
    struct file* f;
    
    lock_acquire(&FSLock);

    f = GetOpenFile(fd);

    if(f != NULL)
    {
        file_seek(f, position);
    }

    lock_release(&FSLock);
    return;
}

/*
    Returns the position in bytes from the start of the file
*/
unsigned tell (int fd) //11
{
    unsigned pos = 0;

    struct file* f;

    lock_acquire(&FSLock);

    f = GetOpenFile(fd);

    if(f != NULL)
    {
        pos = (unsigned)file_tell (f);
        lock_release(&FSLock);
        return pos;
    }

    lock_release(&FSLock);
    return 0;
}

/*
    closes file at fd
*/
void close (int fd) //12
{
    struct list_elem *currentelem, *prevelem;

    struct openfiles *of;

    lock_acquire(&FSLock);

    currentelem = list_end (&openfilelist);

    while (currentelem != list_head (&openfilelist))
    {
        prevelem = list_prev (currentelem);
        of = list_entry (currentelem, struct openfiles, _elem);

        if (of->_fd == fd)
        {
            file_close (of->_file); //Panic here due to corrupted inode open counter
            list_remove (currentelem);
            lock_release(&FSLock);
            return;
        }

        currentelem = prevelem;
    }    Reads from the fd 
    into the buffer pointer
    for size amount

    lock_release(&FSLock);
    return;
}
