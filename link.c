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
extern char line[256], cmd[32], pathname[256], newfile[256];

int link()
{

char parent[128], temp[128], child[128];
//Setting paths for parent and new file
strcpy(temp, newfile);
strcpy(parent, dirname(temp));
strcpy(temp, newfile);
strcpy(child, basename(temp));
//MINODES for old file and parent dir
MINODE *mip, *pmip;
//Get the ino for file we want to link to
//get inumber of the pathname
int ino = getino(pathname);//Check that this found a valid ino
	if(ino == 0)
	{
	printf("Invalid Path\n");
	return -1;
	}
//Get MINODE for this files ino
mip = iget(dev, ino);
INODE *ip= &mip->INODE;
//Check file is not a dir
	if(S_ISDIR(ip->i_mode))
	{
	printf("Cannot Link to Directory\n");
	//Put minode back and return
	iput(mip);
	return -1;
	}
	//Get inode for dir where link will be created
	int pino = getino(parent);
	//Check that this is a valid directory
	if(pino == 0)
		{
		printf("Invalid Path\n");
		iput(mip);
		return -1;
		}
	//Get MINODE for the parent dir
	pmip = iget(dev, pino);
	//Make sure the new file path does not exist
	if (search(pmip, child) != 0)
		{
		printf("File already exists\n");
		iput(mip);
		iput(pmip);
		return -1;
		}
	//Create the new file link
	enter_name(pmip, ino, child);
	//Parent MINODE touch time and make dirty
	pmip->INODE.i_atime = time(0L);
	pmip->dirty = 1;
	//Put parent MINODE back
	iput(pmip);
	//increment links count by 1 for the old file
	ip->i_links_count++;
	//Put back old file MINODE
	iput(mip);
}



int symlink(){MINODE *mip, *pmip;
int len;
char parent[128], temp[128], child[128];
//Setting pathname for child and parent
strcpy(temp, newfile);
strcpy(parent, dirname(temp));
strcpy(temp, newfile);
strcpy(child, basename(temp));
//Get the ino for file we want to link to
//get inumber of the pathname
int ino = getino(pathname);
//Check that this found a valid ino
if(ino == 0){
printf("Invalid Path\n");
return -1;
}
//Get MINODE for this files ino
mip = iget(dev, ino);
//Check file is a dir or reg file
if(!S_ISDIR(mip->INODE.i_mode) && !S_ISREG(mip->INODE.i_mode)){
printf("Needs to be a directory or file\n");
//Put minode back and return
iput(mip);
return -1;
}
//Put old file mip back
iput(mip);
//Get parent inode of newfile
int pino = getino(parent);
pmip = iget(dev, pino);
INODE *pip = &pmip->INODE;
//Make sure parent path for new file is a dir
if(!S_ISDIR(pip->i_mode)){
printf("Invalid Path\n");
iput(pmip);
return -1;
}
//Make sure a file does not already exist
if(search(pmip, child) != 0){
printf("File already exists\n");
iput(pmip);
return -1;
}
//Create new inode for file
//create(pmip, child);
ino = ialloc(dev);//Get MINODE of the new file
mip = iget(dev, ino);
INODE *ip = &mip->INODE;
len = strlen(pathname);
//Write contents to make it a link inode
ip->i_mode = 0120000; // link mode
ip->i_uid = running->uid; // Owner uid
//ip->i_gid = running->gid; // Group Id
ip->i_size = len; // Size in bytes
ip->i_links_count = 1;
// Links count=1 because of ..
ip->i_atime = time(0L); // set to current time
ip->i_ctime = time(0L);
ip->i_mtime = time(0L);
ip->i_blocks = 2;
//Copy pathname into inode i_block
strncpy((char *)mip->INODE.i_block, pathname, len);
for(int i = 1; i < 15; i++)
ip->i_block[i] = 0;
//Write new inode out to disk
mip->dirty = 1;
iput(mip);
//Enter name of the file into the directory inode
enter_name(pmip,ino,newfile);
//Touch the parent inode and put it back
pip->i_atime = time(0L);
pmip->dirty = 1;
iput(pmip);
//write back to disk
iput(mip);
}
