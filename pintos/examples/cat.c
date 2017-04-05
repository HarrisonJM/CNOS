/* cat.c

   Prints files specified on command line to the console. */

#include <stdio.h>
#include <syscall.h>

int syscalltest(int argc, char *argv[]);

int
main (int argc, char *argv[]) 
{
  bool success = true;
  int i;
  
  for (i = 1; i < argc; i++) 
    {
      int fd = open (argv[i]);
      if (fd < 0) 
        {
          printf ("%s: open failed\n", argv[i]);
          success = false;
          continue;
        }
      for (;;) 
        {
          char buffer[1024];
          int bytes_read = read (fd, buffer, sizeof buffer);
          if (bytes_read == 0)
            break;
          write (STDOUT_FILENO, buffer, bytes_read);
        }
      close (fd);
    }

  syscalltest(argc, argv);

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

int syscalltest (int argc, char *argv[])
{
        int fd = open(argv[1]); //open file

        int fd2 = open(argv[2]); //open file

        printf("File size: %d\n", filesize(fd)); //filesize

        seek(fd, 20); //SET FILE POSITION

        printf("Tell: %d\n", tell(fd)); //tell

        char testy[35] =  "I HAVE JUST INSERTED THIS TEXT!\n";

        write(fd, testy, sizeof(testy));

	seek(fd, 0);

//	create("test2cp", 1024);

        for (;;)
        {
          char buffer[1024];
          int bytes_read = read (fd, buffer, sizeof buffer);
          if (bytes_read == 0)
            break;
          write (STDOUT_FILENO, buffer, bytes_read);
        }

        for (;;)
        {
          char buffer[1024];
          int bytes_read = read (fd2, buffer, sizeof buffer);
          if (bytes_read == 0)
            break;
          write (STDOUT_FILENO, buffer, bytes_read);
	}

//	remove(argv[1]);

        close(fd);
        close(fd2);

	return 1;
}

