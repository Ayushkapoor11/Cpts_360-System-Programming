/************* mkdir_creat.c file **************/
/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];




int make_dir()
{
//Global pathname holds the pathname to create the dir
//Define start and parent MINDOE
MINODE *start, *pip;
int pino;
char parent[64], child[64], temp[64];
//Get parent path and new dir name
strcpy(temp, pathname);
//Parent directory is this part of path
strcpy(parent, dirname(temp));
strcpy(temp, pathname);
//This would be the dir you are trying to make
strcpy(child, basename(temp));
//Determine if the path is relative or absolute
//Define MINODE start as cwd or root, and use its correspending dev
//This check really only matters if you are using multiple disks
if(parent[0] == '/')
	{
	//Path is absolute
	start = root;
	dev = root->dev;
	}
else
	{
	//Path is relative
	start = running->cwd;
	dev = running->cwd->dev;
	}
//Getting MINODE of the parent with pino
//Get inode of the parent (number in array minode)
pino = getino(parent);
//Get corresponding minode pointer to pino
pip = iget(dev,pino);
//Verify that parent is a directory
if(!S_ISDIR(pip->INODE.i_mode))
	{
	printf("Error - not a directory\n");
	iput(pip);
	return;
	}
//Check if inode for this pathname already exists (child)
if(getino(pathname) != 0)
	{
	printf("Error - directory already exists\n");
	iput(pip);
	return;
	}
//Call mymkdir on parent and child
mymkdir(pip, child);
//Incremnt links, atime and mark as dirty
pip->INODE.i_links_count++;
pip->INODE.i_atime = time(0L);
pip->dirty = 1;
//update pip on disk
iput(pip);
}


//Takes the parent MINODE and the name of the child to be created
int mymkdir(MINODE *pip, char *name)
{
//Initial Varaibales
MINODE *mip;
char buf[BLKSIZE];
DIR *dp;
char *cp;
//Allocate an inode and disk block for the new directory
int ino = ialloc(dev);
int bno = balloc(dev);
//Load inode of new directory
//iget will create the new minode in the minode array
mip = iget(dev,ino);
INODE *ip = &mip->INODE;
//Write contents to make it a dir inode
ip->i_mode = 0x41ED; // dir mode
ip->i_uid = running->uid; // Owner uid
//Group id is not in our process struct, this was provided
//ip->i_gid = running->gid; // Group Id
ip->i_size = BLKSIZE; // Size in bytes
ip->i_links_count = 2;
// Links count=2 because of . and ..
ip->i_atime = time(0L); // set to current time
ip->i_ctime = time(0L);
ip->i_mtime = time(0L);
ip->i_blocks = 2;
// LINUX: Blocks count in 512-byte chunks
ip->i_block[0] = bno;
// new DIR has one data block
for(int i = 1; i < 15; i++)
ip->i_block[i] = 0;
//Write new inode out to disk
mip->dirty = 1;
iput(mip);
// Now write write . and ..
//dir starts at ., get its ino, name, and length
dp = (DIR *)buf;
dp->inode = ino;
strncpy(dp->name, ".", 1);
dp->name_len = 1;
dp->rec_len = 12;
//Set cp and increment to the next dir
cp = buf;
cp += dp->rec_len;
//Now ready for next entry
dp = (DIR *)cp;
//Now at .. dir
dp->inode = pip->ino;
strncpy(dp->name, "..", 2);
dp->name_len = 2;
dp->rec_len = 1012;
//Now use buf to write to disk block buf
put_block(dev, bno, buf);
//Enter name of the new dir into parent directory
enter_name(pip, ino, name);
}

int creat_file()
{
//Define start and parent inode
MINODE *start, *pip;
int pino;
char parent[64], child[64], temp[64];
//Get parent path and new dir name
strcpy(temp, pathname);
//Parent directory is this part of path
strcpy(parent, dirname(temp));
strcpy(temp, pathname);
//This would be the dir you are trying to make
strcpy(child, basename(temp));
//Determine if the path is relative or absolute
//Define MINODE start as cwd or root, and declare it correspending dev
if(parent[0] == '/')
	{
	//Path is absolute
	start = root;
	dev = root->dev;
	}
else
	{
	//Path is relative
	start = running->cwd;
	dev = running->cwd->dev;
	}
//Getting MINODE of the parent with pino
//Get inode of the parent (number in array minode)
pino = getino(parent);//Get corresponding minode pointer to pino
pip = iget(dev,pino);
//Verify that parent is a directory and no child exists
if(!S_ISDIR(pip->INODE.i_mode))
	{
	printf("Error - not a directory\n");
	iput(pip);
	return;
	}
if(getino(pathname) != 0)
	{
	printf("Error - directory already exists\n");
	iput(pip);
	return;
	}
//Call create on parent and child
my_creat(pip, child);
//Incremnt atime and mark as dirty
pip->INODE.i_atime = time(0L);
pip->dirty = 1;
//update pip on disk
iput(pip);
}  


int my_creat(MINODE *pip, char *name)
{
MINODE *mip;
char buf[BLKSIZE];
DIR *dp;
char *cp;
int rec_length, myino;

//Allocate an inode for the new file
int ino = ialloc(dev);
//Load inode of new directory
mip = iget(dev,ino);
INODE *ip = &mip->INODE;
//Write contents to make it a file inode
ip->i_mode = 0x81A4; // dir mode
ip->i_uid = running->uid; // Owner uid
//ip->i_gid = running->gid; // Group Id
ip->i_size = 0; // Size in bytes
ip->i_links_count = 1;
// Links count=1 because of ..
ip->i_atime = time(0L); // set to current time
ip->i_ctime = time(0L);
ip->i_mtime = time(0L);
ip->i_blocks = 2;
// LINUX: Blocks count in 512-byte chunks
for(int i = 1; i < 15; i++)
ip->i_block[i] = 0;//Write new inode out to disk
mip->dirty = 1;
iput(mip);
//Enter name of the file into the directory inode
enter_name(pip,ino,name);
}
