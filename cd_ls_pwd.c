
/************* cd_ls_pwd.c file **************/




/**** globals defined in main.c file ****/

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

chdir(){//Temp is used to copy pathname and mip used to hold dir and change
char temp[256];
MINODE *mip;
int dev, ino;

//If a path was not provided use cwd
if (pathname[0] == 0){
iput(running->cwd);
running->cwd = iget(root->dev, 2);
return;
}

//Use roots device number if pathname is absolute

if (pathname[0] == '/'){
dev = root->dev;
} else {
//Use cwd's device number if pathname is relative
dev = running->cwd->dev;
}

//Get the inumber of the provided pathname
strcpy(temp, pathname);
ino = getino(temp);
//If the ino is 0 and not found than pathname is invalid
if (!ino)
{
printf("Invalid pathname\n");
return(-1);
}
//Printing inode and dev number
printf("dev=%d ino=%d\n", dev, ino);
//Get the MINODE of the ino number
mip = iget(dev, ino);
//Make sure that the path is to a directory and not a file
if (!S_ISDIR(mip->INODE.i_mode))
{
printf("Not a directory\n");
iput(mip);
return(-1);
}
//Write back to the disk and change cwd to change the directory
iput(running->cwd);
running->cwd = mip;
}
int ls_file(int ino, char *name){
MINODE *mip;
//Get the MINODE and inode of the current ino
mip = iget(dev, ino);
INODE *ip = &(mip->INODE);
//Print the ino number other stats and the name
printf("%4d %4d %4d %4d %s\n", ino, ip->i_mode, ip->i_uid, ip->i_size, name);return 0;
}
int ls_dir(){
int ino;
MINODE *mip;
char buf[BLKSIZE], name[256], temp[256], wd[256], *cp;
DIR *dp;
//Getting the path to where to ls based on pathname
strcpy(temp, pathname);
strcpy(wd, dirname(temp));
ino = getino(wd);
//Check if inode is not found
if(ino == 0){
printf("Error - No path");
return 0;
}
//Get MINODE based on the ino of dir of pathname
mip = iget(dev,ino);
//Get its first block
get_block(mip->dev, mip->INODE.i_block[0], buf);
dp = (DIR *)buf;
cp = buf;
//Increment through to each entry and call ls_file
while (cp < buf + BLKSIZE){
//Get the ino and name to pass to ls_file
strncpy(name, dp->name, dp->name_len);
name[dp->name_len] = 0;
ls_file(dp->inode, name);
//Increment character and directory pointer
cp += dp->rec_len;
dp = (DIR*)cp;
}
//return
return 0;
}
// Recursively prints working directory
int rpwd(MINODE *wd)
{
char buf[BLKSIZE], name[256], *cp;
DIR *dp;
MINODE *parent;
int ino, pino;
//If working directory is the root then return
if (wd == root)
return;//Get the first block of dir and look at second entry, ..
get_block(dev, wd->INODE.i_block[0], buf);
dp = (DIR *)buf;
cp = buf;
//Get ino using first entry
ino = dp->inode;
//Increment to second entry to get parent ino
cp += dp->rec_len;
dp = (DIR *)cp;
pino = dp->inode;
//Get the parent MINODE using inumber
parent = iget(dev, pino);
//Get the parent block to find the childs name (current wd)
get_block(parent->dev, parent->INODE.i_block[0], buf);
dp = (DIR *)buf;
cp = buf;
while (cp < buf + BLKSIZE){
//Check to see if dirs ino is equal to current
if(dp->inode == ino){
strncpy(name, dp->name, dp->name_len);
name[dp->name_len] = 0;
}
//Increment character and directory pointer
cp += dp->rec_len;
dp = (DIR*)cp;
}
//Recursive call on parent
rpwd(parent);
//Put the block back into memory and print its name
iput(parent);
printf("/%s", name);
return 1;
}
int pwd(MINODE *wd){
//If working directory is root print '/'
if (wd == root){
printf("/\n");
return;
}
//If its not root recursively print directories
rpwd(wd);
printf("\n");
}
