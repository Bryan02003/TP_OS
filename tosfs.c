#define FUSE_USE_VERSION 31

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
#include <fuse_lowlevel.h>



static int tosfs_stat(fuse_ino_t ino, struct stat *stbuf, struct tosfs_inode *inodes)
{
    struct tosfs_inode *inode = &inodes[ino];
    if (!inode)
    {
        return -1;
    }


    stbuf->st_ino = inode->inode;
    stbuf->st_mode = inode->mode;
    stbuf->st_nlink = inode->nlink;
    stbuf->st_size = inode->size;

    return 0;
}

struct tosfs_state
{
    struct tosfs_superblock* superblock;
    struct tosfs_inode* inodes;
};

static void tosfs_ll_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    struct stat stbuf;
    struct tosfs_state *ts = fuse_req_userdata(req);  // Charger l'état

    (void) fi;
    memset(&stbuf, 0, sizeof(stbuf));

    if (tosfs_stat(ino, &stbuf, ts->inodes) == -1)
    {
        fuse_reply_err(req, ENOENT);
    }
    else
    {
        fuse_reply_attr(req, &stbuf, 1.0);
    }
}

static struct fuse_lowlevel_ops tosfs_ll_oper =
{
	//.lookup		= tosfs_ll_lookup,
	.getattr	= tosfs_ll_getattr,
	//.readdir	= tosfs_ll_readdir,
	// .open		= tosfs_ll_open,
	// .read		= tosfs_ll_read,
};

int main(int argc, char *argv[]) 
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <point_de_montage>\n", argv[0]);
        return 1;
    }

    int fd = open("test_tosfs_files", O_RDONLY);
    if (fd == -1) {
        perror("Erreur d'ouverture du fichier");
        return 1;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Erreur lors du fstat");
        close(fd);
        return 1;
    }

    void *mapped_fs = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_fs == MAP_FAILED) {
        perror("Erreur lors du mmap");
        close(fd);
        return 1;
    }

    struct tosfs_superblock *sb_ptr = (struct tosfs_superblock *)mapped_fs;
    struct tosfs_state state = {
        .superblock = sb_ptr,
        .inodes = (struct tosfs_inode *)((char *)mapped_fs + sizeof(struct tosfs_superblock)) 
    };

    printf("Magic number : %u\n", sb_ptr->magic);
    printf("Nombre d'inodes : %u\n", sb_ptr->inodes);
    printf("Nombre de blocs : %u\n", sb_ptr->blocks);

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_chan *ch;
    char *mountpoint;
    int err = -1;

    if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) == -1) {
        fprintf(stderr, "Erreur de parsing des arguments de la ligne de commande FUSE\n");
        return 1;
    }

    ch = fuse_mount(mountpoint,NULL);
    if (ch == NULL) {
        perror("Erreur lors du montage du système de fichiers");
        return 1;
    }

    struct fuse_session *se = fuse_lowlevel_new(&args, &tosfs_ll_oper, sizeof(tosfs_ll_oper), &state);
    if (!se) {
        fprintf(stderr, "Erreur lors de la création de la session FUSE\n");
        fuse_unmount(mountpoint,ch);
        return 1;
    }

    fuse_session_add_chan(se, ch);

    err = fuse_session_loop(se);

    fuse_session_destroy(se);
    fuse_unmount(mountpoint, ch);
    munmap(mapped_fs, sb.st_size);
    close(fd);
    fuse_opt_free_args(&args);

    return err ? 1 : 0;
}

https://github.com/Bryan02003/TP-OS