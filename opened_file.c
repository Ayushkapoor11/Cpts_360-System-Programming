extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256], newfile[256];

int open_file()
{
int mode, ino;
MINODE *mip;
OFT *oft;
//extracting the mode
sscanf(newfile,"%d", &mode);
sscanf("newfile :%d", &mode);
//set the dev
//if path starts from root
if(pathname[0] == '/')
	{
	dev = root->dev;
	}
//if path starts from relative
else
	{
	dev = running->cwd->dev;
	}
//get ino number for the path
ino= getino(pathname);
//return if invalid path
if(ino==0)
	{
	printf("Invalid pathname \n");
	return -1;
	}
//Get the minode of the path
mip = iget(dev, ino);
INODE *ip = &mip->INODE;
//Check to make sure it is a regular file
if(!S_ISREG(ip->i_mode))
	{
	printf("Not a regular file \n");
	iput(mip);
	return -1;
	}
//Check if file is already opened in incompatible mode
for(int i = 0; i < NFD; i++)
	{
	//Check for this mptr to see if its already opened
	if(running->fd[i]!=NULL && running->fd[i]->mptr == mip)
		{
		//Check to see if its opened in a mode other than read
		if(running->fd[i]->mode > 0 || mode > 0)
			{
			printf("file already opened for writing \n");
			iput(mip);
			return -1;
			}
		//Check to see if both are to be opened for read
		if(running->fd[i]->mode == 0 || mode == 0)
			{
			//Increment refcount and return
			running->fd[i]->refCount++;
			iput(mip);
			return i;
			}
		}
	}
//Allocate a new open file table
oft = malloc(sizeof(OFT));
//Set the initial values (oft entries)
oft->mode = mode;
oft->refCount = 1;
oft->mptr = mip;
switch(mode)
{
	//read
	case 0:
	oft->offset = 0;
	break;
	//write
	case 1:
	truncate(mip); //shorten
	oft->offset = 0;
	break;
	//read and write
	case 2:
	oft->offset = 0;
	break;
	//append
	case 3:
	oft->offset = ip->i_size;
	break;
	
	default:
	printf("Invalid mode! \n");
	iput(mip);
	return -1;
}
//Look for an empty process slot
int i = 0;
for(i = 0 ; i < NFD; i++)
	{
	if(running->fd[i] == NULL)
		{
		break;
		}
	}
//Put the oft into the empty slot
running->fd[i] = oft;
//if mode is read, then just change the access time
if(oft->mode == 0)
	{
	ip->i_atime = time(0L);
	}
//if mode is other than read mode
else if(oft->mode > 0)
	{
	ip->i_atime = time(0L);
	ip->i_mtime = time(0L);
	}
//mark the file dirty
mip->dirty = 1;
//put minode back on disk
iput(mip);
printf("file opened");
mypfd();
return i;
}

int mypfd()
{
printf(" OPENED FILES: \n");
printf("fd mode offset INODE refcounts\n");
//Search through fd to find any OFT's
for(int i = 0; i < NFD; i++)
	{
		char mode[8];
		//Check OFT exist and convert mode from int
		if(running->fd[i] != NULL) 
		{
			if(running->fd[i]->mode == 0)
			strcpy(mode, "READ");
			else if(running->fd[i]->mode == 1)
			strcpy(mode, "WRITE");
			else if(running->fd[i]->mode == 2)
			strcpy(mode, "R/W");
			else if(running->fd[i]->mode == 3)
			strcpy(mode, "APPEND");
			//Print all the info for the OFT
			printf("%d %s %d)[%d, %d] %d\n", i, mode, running->fd[i]->offset,
			running->fd[i]->mptr->dev, running->fd[i]->mptr->ino, running->fd[i]->refCount);
		}
	}
}

int close_file(int fdnum)
	{
	MINODE *mip;
	//Check if fd is in the valid range and exists
	if(fdnum < 0 || fdnum > NFD || running->fd[fdnum] == NULL)
	{
		printf("fd out of range or does not exist \n");
		return -1;
	}
	//Check if this is the last reference to OFT
	if(running->fd[fdnum]->refCount == 1)
		{
		//Last one so remove OFT
		running->fd[fdnum]->refCount--;
		mip = running->fd[fdnum]->mptr;
		iput(mip);
		running->fd[fdnum] = 0;
		} 
	else 
		{
		//Decrement refCounts
		running->fd[fdnum]->refCount--;
		return 0;
		}
	mypfd();
}

int mylseek(int fdnum, int position)
{
int op;
//Check to make sure OFT exist and is valid
if(running->fd[fdnum] != NULL && fdnum < 8)
	{
	//Check to make sure it does not overrun end of file
	if(running->fd[fdnum]->mptr->INODE.i_size < position)
	{
		printf("The offset is greater than file size\n");
		return -1;
	}
	//Save the original offset and set it to position
	op = running->fd[fdnum]->offset;
	running->fd[fdnum]->offset = position;
	} 
	else
	{
	printf("Invalid lseek call \n");
	return -1;
	}
return op;
}
