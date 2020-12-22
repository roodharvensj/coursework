/*
 * Header file for myio.c
 * This file simply lists all function prototypes (aka definitions)
 * Use include "myio.h" (note the use of quotation marks and not <> brackets)
 * in any local .c file that should call these functions (including myio.c).
 */

int myopen(const char *pathname, const int flags);
int myread(int fd, void *buf, int count);
int mywrite(int fd, const void *buf, int count);
int myseek(int fd, int offset, int whence);
int myflush(int fd);
int myclose(int fd);
int myflush(int fd);
int getIndexOf(int fd);
