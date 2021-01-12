extern MINODE minode[NMINODE];extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256], newfile[256];
void file_stat()
{

struct stat myst;

//get INODE of filename into memory:

int ino = getino(pathname);
MINODE *mip = iget(dev, ino);
myst.st_dev = mip->dev;
myst.st_ino = ino;

//Add the stats of the inode
INODE *ip = &mip->INODE;
//Write contents to make it a dir inode
myst.st_mode = ip->i_mode;
// dir mode
myst.st_uid = ip->i_uid;
// Owner uid
//ip->i_gid = running->gid;
// Group Id
myst.st_size = ip->i_size;
// Size in bytes
//myst.st_links_count = ip->i_links_count;
// Links count=1 because of ..
myst.st_atime = ip->i_atime; // set to current time
myst.st_ctime = ip->i_ctime;
myst.st_mtime = ip->i_mtime;
myst.st_blocks = ip->i_blocks;
//Put mip back
iput(mip);
//print the stats to console
printf("dev: %d\n", myst.st_dev);
printf("ino: %d\n", myst.st_ino);
printf("mode: %d\n", myst.st_mode);
printf("uid: %d\n", myst.st_uid);
printf("Size: %d\n", myst.st_size);
printf("i_atime: %d\n", myst.st_atime);
printf("i_ctime: %d\n", myst.st_ctime);
printf("i_mtime: %d\n", myst.st_mtime);
printf("i_blocks: %d\n", myst.st_blocks);
}
