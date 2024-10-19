#include "tosfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>
//#include <fuse_lowlevel.h>


int main()
{
    int fd = open("test_tosfs_files", O_RDONLY);
    if (fd == -1) 
    {
        perror("Erreur d'ouverture du fichier");
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) 
    {
        perror("Erreur lors du fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }

    void *mapped_fs = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_fs == MAP_FAILED) 
    {
        perror("Erreur lors du mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    struct tosfs_superblock *sb_ptr = (struct superblock *)mapped_fs;

    printf("Magic number : %u\n", sb_ptr->magic);
    printf("Inodes number : %u\n", sb_ptr->inodes);
    printf("Blocks number : %u\n", sb_ptr->blocks);

    munmap(mapped_fs, sb.st_size);
    close(fd);
    
    return 0;

}
