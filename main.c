
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>
#include "type.h"
	
MINODE minode[NMINODE];
MINODE *root;
PROC proc[NPROC], *running;
char gpath[256]; // global for tokenized components
char *name[64]; // assume at most 64 components in pathname
int n;
// number of component strings
int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start;
char line[256], cmd[32], pathname[256], newfile[256];
#include "util.c"
#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link.c"
#include "unlink.c"
#include "misc.c"
#include "opened_file.c"
#include "read_file.c"
#include "write.c"

//This function serves to initialize all the instances of the two defined datatypes minode and proc
int init()
{
int i, j;
MINODE *mip;
PROC *p;
printf("init()\n");
//Initializes and freeing the inodes
//NMINODE is 64
//minode array definded above as minode
for (i=0; i<NMINODE; i++)
{
	//set mip to dereferenced minode at this i
	mip = &minode[i];
	//Initializes all of minodes value
	mip->dev = mip->ino = 0;
	mip->refCount = 0;
	mip->mounted = 0;
	mip->mptr = 0;}
	//Initializes the process (2)
	//NPROC is 2
	//proc array defined above
	for (i=0; i<NPROC; i++)
	{
		//access pointer to process in array
		p = &proc[i];
		//Initializing the base process values
		p->pid = i;
		p->uid = 0;
		p->cwd = 0;
		p->status = FREE;
		for (j=0; j<NFD; j++)
		p->fd[j] = 0;
	}
}
// load root INODE and set root pointer to it
int mount_root()
{
	printf("mount_root()\n");
	root = iget(dev, 2);
}
char *disk = "mydisk";
int main(int argc, char *argv[ ])
{
	int ino;
	char buf[BLKSIZE];
	if (argc > 1)
	disk = argv[1];
	//Checking Disk opens properly
	printf("checking EXT2 FS ....");
	if ((fd = open(disk, O_RDWR)) < 0)
	{
		printf("open %s failed\n", disk); exit(1);
	}
	//Dev is pointing to the disk we read in
	dev = fd;
	/********** read super block at 1024 ****************/

	get_block(dev, 1, buf); 
	sp = (SUPER *)buf;
	/* verify it's an ext2 file system *****************/
	if (sp->s_magic != 0xEF53)
	{
		printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
		exit(1);
	}
printf("OK\n");
//Geting overall inode and block count from the super block
ninodes = sp->s_inodes_count;
nblocks = sp->s_blocks_count;//Read in group descriptor (2nd block) blocks 3-7 also reserved for this
get_block(dev, 2, buf);
gp = (GD *)buf;
//Get the inode map and the block map
bmap = gp->bg_block_bitmap;
imap = gp->bg_inode_bitmap;
//Get the value where the inode starts
inode_start = gp->bg_inode_table;
printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);
init();
mount_root();
//This should always print 1 after root is initialized
printf("root refCount = %d\n", root->refCount);
printf("creating P0 as running process\n");
//Accessing first proc in process array and preparing values
running = &proc[0];
running->status = READY;
//grabbing root that was just initialized setting as cwd for this process
running->cwd = iget(dev, 2);
//This should also print 1
printf("root refCount = %d\n", root->refCount);
//printf("hit a key to continue : "); getchar();
while(1)
{
	printf("input command : [ls|cd|pwd|mkdir|creat|open|close|pfd|cat|cp|rmdir|link|symlink|unlink|quit] ");
	fgets(line, 128, stdin);
	line[strlen(line)-1] = 0;
	if (line[0]==0)
	continue;
	pathname[0] = 0;
	cmd[0] = 0;
	sscanf(line, "%s %s %s", cmd, pathname, newfile);
	printf("cmd=%s pathname=%s newfile =%s\n", cmd, pathname, newfile);
	if (strcmp(cmd, "ls")==0)
	ls_dir();
	if (strcmp(cmd, "cd")==0)
	chdir();
	if (strcmp(cmd, "pwd")==0)
	pwd(running->cwd);
	if (strcmp(cmd, "mkdir")==0)
	make_dir();
	if (strcmp(cmd, "rmdir")==0)
	rmdir();
	if (strcmp(cmd, "creat")==0)
	creat_file();
	if (strcmp(cmd, "stat")==0)
	file_stat();if (strcmp(cmd, "link")==0)
	link();
	if (strcmp(cmd, "symlink")==0)
	symlink();
	if (strcmp(cmd, "unlink")==0)
	unlink();
	if (strcmp(cmd, "pfd")==0)
	mypfd();
	if (strcmp(cmd, "open")==0)
	open_file();
	if (strcmp(cmd, "cat")==0)
	mycat();
	if (strcmp(cmd, "cp")==0)
	mycp();
	if (strcmp(cmd, "close")==0)
	{
		int fdnum;
		sscanf(pathname, "%d", &fdnum);
		close_file(fdnum);
	}
	if (strcmp(cmd, "lseek")==0)
	{
		int fdnum, offset;
		sscanf(pathname, "%d", &fdnum);
		sscanf(newfile, "%d", &offset);
		mylseek(fdnum, offset);
	}
	if (strcmp(cmd, "quit")==0)
	quit();
}
}
//This simply puts all of the MINODES back onto the disk to make sure its all updated and closes program
int quit()
{
int i;
MINODE *mip;
for (i=0; i<NMINODE; i++)
	{
	mip = &minode[i];
	if (mip->refCount > 0)
	iput(mip);
	}
exit(0);
}
