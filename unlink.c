/************* link.c file **************/
/**** globals defined in link.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

int unlink()
{

MINODE *mip, *pip;
char parent[64], child[32], temp[64];
//Check to make sure pathname is provided
if(strcmp(pathname, "") == 0)
	{
	printf("Error - No Pathname\n");
	return -1;

	}
//Get the MINODE of the provided pathname
int ino = getino(pathname);
//Check to make sure the ino is valid
if(ino == 0)
	{
	printf("Invalid Pathname\n");
	return -1;
	}
//Get the MINODE and inode of the pathname
mip = iget(dev, ino);
//Check to make sure the inode is a file
if(S_ISDIR(mip->INODE.i_mode))
	{
	printf("Error - Cannot unlink directory\n");
	iput(mip);
	return -1;
	}
//Ready to unlink the file
//Decrement link count of the pathname inode
mip->INODE.i_links_count--;
//If link count is 0 free the inode and its blocks
if(mip->INODE.i_links_count == 0)
	{
	truncate(mip);
	idalloc(mip->dev,mip->ino);
	}
//Put mip back to the blocks
iput(mip);
//Get the parent and child path based on the pathname
strcpy(temp, pathname);
strcpy(parent, dirname(temp));
strcpy(temp, pathname);
strcpy(child, basename(temp));
printf("\nparent: %s child: %s\n",parent,child);
//Get the parent MINODE of the file we are unlinking
int pino = getino(parent);
pip = iget(dev, pino);
//Call rm_child on this dir for this child
rm_child(pip, child);
//Put pip back to the blocks
iput(pip);
}
