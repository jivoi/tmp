// Задача на именованные каналы.

// Напишите программу, которая создает два именованных канала - /home/box/in.fifo и /home/box/out.fifo

// Пусть программа читает in.fifo и все прочитанное записывает в out.fifo.

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_BUF 2048

int main()
{
    char * outfifo = "/home/box/out.fifo";
    char * infifo = "/home/box/in.fifo";

    /* create the FIFO (named pipe) */
    mkfifo(infifo, 0666);
    mkfifo(outfifo, 0666);

    int outfd = open(outfifo, O_WRONLY);
    perror("Error after outfd open: ");
    int infd = open(infifo, O_RDONLY | O_NONBLOCK);
    perror("Error after infd open: ");

    char buf[MAX_BUF];

    while ( int got = read(infd, buf, MAX_BUF) ) {
        printf("Read: %s\n", bf);
        write(outfd, buf, got);

        memset(buf, 0, 12);
    }

    close(outfd);
    close(infd);

    /* remove the FIFO */
    unlink(infifo);
    unlink(outfifo);

    return 0;
}