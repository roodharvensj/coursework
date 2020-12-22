/*
 * mycopyfile.c
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "myio.h"

#define BYTES_PER_ITERATION 1024

void usage(char *argv[]);

int main(int argc, char *argv[])
{
    char *src_filename, *dst_filename;
    int src_fd, dst_fd;
    int n;
    char buf[BYTES_PER_ITERATION];

    /* check command-line args */
    // if (argc < 3) {
    //     usage(argv);
    //     exit(1);
    // }
    src_filename = "file2.txt"; //argv[1];
    dst_filename = "file2copy.txt"; //argv[2];

    /* open source and destination file */
    src_fd = myopen(src_filename, O_RDONLY);
    if (src_fd < 0) {
        perror("open");
        exit(2);
    }
    dst_fd = myopen(dst_filename, O_CREAT | O_WRONLY);
    if (dst_fd < 0) {
        perror("open");
        exit(3);
    }


    /* read from source into destination */
    while ((n = myread(src_fd, buf, BYTES_PER_ITERATION)) > 0) {
        //printf("\nBuffer from src_fd: \n %s \n\n",buf);
        n = mywrite(dst_fd, buf, n);
        
        if (n < 0) {
            perror("write");
            exit(4);
        }
    }

    if (n < 0) {
        perror("read");
    }

    /* clean up */
    myclose(src_fd);
    myclose(dst_fd);


    printf("Copy Success!\n");


}

void usage(char *argv[])
{
    printf("usage: %s src_filename dst_filename\n", argv[0]);
}
