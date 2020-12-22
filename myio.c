/*
 * Assignment 2: Buffered I/O
 * John Cambefort & Roodharvens Joseph
 * myio.c
 */

#include "myio.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#define MAX_ARRAY_SIZE 1000 
#define MAX_BUFFER_SIZE 4096

struct FileInfo {
    int srcfd; // fd received from open syscall
    int index; // current index within the file
    char *file_data; // buffer for bytes to/from the file 
    int file_pointer; // can only increase: number of bytes read so far
    int w; // Binary for whether there are bytes that haven't been written to the buffer
    int low_bound; // start file index of every buffer
};

struct FileInfo *file_arr[MAX_ARRAY_SIZE];


// declare locally used function
int getIndexOf(int fd);  //Receive index of fd within file_arr
int myflush(int fd); //Call write system call for any unwritten bytes
int mysyscall(struct FileInfo *file, char *string); //checks whether we've reached the end of the buffer 
                                                    //or surpassed the file pointer and 
                                                    //calls write if string == "write" and read if string == "read"


/*
 * Returns a positive int file descriptor on successful open and -1 on error.
 * Stores a pointer to the FileInfo object in the file_arr on success only.
 * Initializes the FileInfo object's srcfd to be used by other I/O functions.
 * The flags integer should be built by or'ing itself with any of open's flags.
 * See man 2 open for more on valid flags.
 */
int myopen(const char *pathname, const int flags) {
    const mode_t mode = 0666;
    int fd;

    //System call to open file
    if((fd = open(pathname, flags, mode)) < 0) {
        perror("open");
        return -1;
    };

    // Get the index i of the file_arr at which we want to insert a FileInfo struct
    int i = fd-3;

    // Malloc space for a FileInfo struct at that index
    file_arr[i] = (struct FileInfo *) malloc(sizeof(struct FileInfo));
    if(file_arr[i] == NULL){
        perror("malloc");
        return -1;
    }

    // Store fd into the FileInfo's srcfd member
    file_arr[i]->srcfd = fd;

    // Initialize everything within 
    file_arr[i]->index = 0;
    file_arr[i]->index = 0;
    file_arr[i]->low_bound = 0;
    file_arr[i]->file_pointer= 0;
    file_arr[i]->file_data = (char *) malloc(MAX_BUFFER_SIZE);
    if(file_arr[i]->file_data == NULL){
        perror("malloc");
        return -1;
    }
    
    return fd; // Return the file descriptor
}

int myread(int fd, void *buf, int count) {
    int local_index, i, j;

    // Get the index i of the file_arr at which we want to insert a FileInfo struct
    if ((i = getIndexOf(fd)) < 0) {
        return -1;
    } 

    // get local index within the buffer (mod the actual index by the buffer size)
    local_index = (file_arr[i]->index % MAX_BUFFER_SIZE);

    //create temp buffer to copy bytes from file_arr[i]->file_data to buf
    char *temp = (char *) malloc(count);
    if(temp == NULL){
        perror("malloc");
        return -1;
    }

    /* Keep copying bytes into temp while:
     * - we haven't hit count yet
     * - j + count doesn't go beyond the buffer size
     * - global index is different from the file pointer value
     */
    j = 0;
    while(j < count && mysyscall(file_arr[i], "read") > 0) {
        
        // Update the real index as bytes are copied out
        temp[j++] = file_arr[i]->file_data[local_index];
        file_arr[i]->index++;
        local_index = (file_arr[i]->index % MAX_BUFFER_SIZE);

    }

    /* Copy out temp to the user's buffer */
    memcpy(buf, temp, j);
    free(temp);
    return j;
}

int mywrite(int fd, const void *buf, int count) {
    int local_index, i, j;

    // Get the index i of the file_arr at which we want to insert a FileInfo struct
    if ((i = getIndexOf(fd)) < 0) {
        return -1; //Exit function if file has not been opened
    }

    // get local index within the buffer (mod the actual index by the buffer size)
    local_index = (file_arr[i]->index % MAX_BUFFER_SIZE);

    char *temp = (char *) malloc(count);
    if(temp == NULL){
        perror("malloc");
        return -1;
    }

    memcpy(temp, buf, count); //copy buf into temp buf


    /* Keep copying bytes from temp while:
     * - we haven't hit count yet
     * - j + count doesn't go beyond the buffer size
     * - global index is different from the file pointer value
     */
    j = 0;
    while(j < count) {

        mysyscall(file_arr[i], "write");

        file_arr[i]->file_data[local_index]=temp[j]; //copy bytes into buffer
        file_arr[i]->w = 1;

        j++;
        file_arr[i]->index++; // Update the real index as bytes are copied out
        local_index = (file_arr[i]->index % MAX_BUFFER_SIZE); //recalculate buffer size

        if(file_arr[i]->file_pointer == (file_arr[i]->index -1) ){ //Increase how far we've reached into the file
        file_arr[i]->file_pointer++;
            }

        mysyscall(file_arr[i], "write");
    }

    free(temp);
    return j;
}


/* Repositions the FileInfo index member:
 * - to offset if whence == SEEK_SET
 * - to offset + the current index if whence == SEEK_CUR
 * Returns the new value of the buffer index on success, -1 on failure.
 */
int myseek(int fd, int offset, int whence) {
    int i, seekset, seekcur;
    
    if ((i = getIndexOf(fd)) < 0){ // prints error if fd does not exist
        printf("Invalid file descriptor: file not found or not open.\n");
        return -1;
    }

    if ((whence < 0) || (whence > 1)) { //prints error if whence is non-binary
        printf("whence must be either 0 (SEEK_SET) or 1 (SEEK_CUR).\n");
        return -1;
    }

    seekset = (whence == SEEK_SET) ? 1 : 0; // SEEK_SET == 0
    seekcur = (whence == SEEK_CUR) ? 1 : 0; // SEEK_CUR == 1
    
    /* Augment index within buffer */
    if (seekset) {

        file_arr[i]->index = offset;

    } else if (seekcur) {

        file_arr[i]->index += offset;

    }
    /* If seek beyond buffer, lseek */
    if (file_arr[i]->low_bound > file_arr[i]->index 
        || (file_arr[i]->low_bound+(MAX_BUFFER_SIZE)) <= file_arr[i]->index 
        || file_arr[i]->file_pointer < file_arr[i]->index){

        if(lseek(fd, file_arr[i]->index, SEEK_SET)<0){
            perror("lseek");
            return -1;
        }
        file_arr[i]->low_bound = file_arr[i]->index;
    }

    return file_arr[i]->index;

}



/* Closes a file descriptor. Returns 0 on success and -1 otherwise.
 * Also clears the appropriate FileInfo object from the file_arr.
 */
int myclose(int fd) {

    int status, i = getIndexOf(fd);

    /* Flush and Clear FileInfo object from file_arr here */
    if(file_arr[i]->w == 1){
        myflush(fd);
    }

    if((status = close(fd)) < 0) {
        perror("close");
        return -1;
    }

    if(file_arr[i]->file_data){
        free(file_arr[i]->file_data); // need to free any file data in our struct
    }

    free(file_arr[i]); // this will free flat variables

    return status;

}

/*  Flushes buffer of file by writing buffered data to its destination.
    Data that's been read into the buffer will be written back to the file.
    Returns the number of files for which data was flushed. */

int myflush(int fd){
    int left_over_bytes, bytes_flushed, check, i = getIndexOf(fd);

    if (i == -1) {  // Get the index i of the file_arr at which we want to insert a FileInfo struct
        return -1; //Exit function if file has not been opened
    }
    
    /* Write out file_data to disk */
    if (file_arr[i]->w == 1) {
        left_over_bytes = (file_arr[i]->index - file_arr[i]->low_bound);
        check=lseek(file_arr[i]->srcfd,file_arr[i]->low_bound,SEEK_SET);
        if(check== -1){
            perror("lseek");
            return -1;
        }
        bytes_flushed = write(file_arr[i]->srcfd,file_arr[i]->file_data, left_over_bytes);
        if(bytes_flushed == -1){
            perror("write");
            return -1;
            }
        file_arr[i]->low_bound = file_arr[i]->index;
        file_arr[i]->w = 0;
        return 0;        
    } 

    return i;
}

/* Returns the index of a FileInfo object in the file_arr,
* when given its file descriptor fd. */
int getIndexOf(int fd) {
    int i=fd-3;
    if(file_arr[i]->srcfd == fd){
        return i;
    }

    printf("File Not Previously Opened\n");
    return -1;
  }


/*Checks whether we've reached the end of the buffer or surpassed the file pointer,
 *Calls write if string == "write" and read if string == "read" */

 int mysyscall(struct FileInfo *file, char *string) {
     int check, bytes=1;

    if(strcmp(string,"write") == 0){
        //First check if we've reached the end of the buffer
        if ( file->low_bound > file->index 
            ||(file->low_bound+(MAX_BUFFER_SIZE)) <= file->index
            || file->file_pointer < file->index){
            check=lseek(file->srcfd,file->low_bound,SEEK_SET); //Seek to low_bound of buffer
            if(check<0){
                perror("lseek");
                return check;
            }
            bytes = write(file->srcfd,file->file_data,MAX_BUFFER_SIZE); // Write out file_data to disk
            if (bytes == 0) {
                file->w = 0; // Set Write bit to 0
                file->low_bound = file->index;
                return 0;
            }else if(bytes < 0){
                printf("\n\nERROR at FD: %d",file->srcfd);
                perror("write");
                printf("\n\n");
                return bytes;
            }
            file->low_bound = file->index; // Update lower_bound
            file->w = 0; // Set Write bit to 0
            }
    }
    if(strcmp(string,"read") == 0){
        /* Flush the buffer if the buffer is filled up, or we reach the end of the file.
         * Fill up the entire buffer, or as much of it as possible, from the File. */
        if (    file->low_bound > file->index 
            || (file->low_bound+(MAX_BUFFER_SIZE)) <= file->index 
            ||  file->file_pointer <= file->index){

            //Flush buffer if there is written content
            if(file->w == 1){
                myflush(file->srcfd);
            }

            //Fill the buffer by reading the bytes
            check=lseek(file->srcfd,file->index,SEEK_SET);
            if(check<0){
                perror("lseek");
                return check;
            }
            bytes = read(file->srcfd, file->file_data, MAX_BUFFER_SIZE);
            // File ends within this buffer size
            if (bytes == 0) {
                file->file_pointer += bytes; //Increase how far we've reached into the file  
                file->low_bound = file->index; //Set lower bound to index
                return bytes;
            }else if(bytes < 0){
                printf("\n\nERROR at FD: %d",file->srcfd);
                perror("read");
                printf("\n\n");
                return bytes;
            }

            file->file_pointer += bytes; //Increase how far we've reached into the file  
            file->low_bound = file->index; //Set lower bound to index
        }
    }
    return bytes;
        
 }


