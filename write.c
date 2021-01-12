extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256], newfile[256];

int mycp()
{
char buf[BLKSIZE];
//get source files inode number
int src_ino = getino(pathname);
//get destination's inode number
int dest_ino = getino(newfile);
//check that destination does not exist
char temp_holder[256];
if (dest_ino == 0)
	{
	// save the original file, since creat uses pathname
	strcpy(temp_holder, pathname);
	// copy destination file's path
	strcpy(pathname,newfile);
	// create the empty file
	creat_file();
	// reset path
	strcpy(pathname, temp_holder);
	}
//open source file
//save the destination file pathname
strcpy(temp_holder,newfile);
//set this to read mode for open call
strcpy(newfile,"0");
//open source file to read
int fdnum = open_file();
//reset newfile
strcpy(newfile,temp_holder);
//open destination file to writestrcpy(temp_holder,pathname);
strcpy(pathname,newfile);
//set to write mode for the open call
strcpy(newfile,"2");
int gd = open_file();
strcpy(newfile,pathname);
strcpy(pathname,temp_holder);

//read in n bytes to buf from old file, then write it to newfile
while (n = myread(fdnum, buf, BLKSIZE))
	{
	mywrite(gd, buf, n);
	}
//close the files
close_file(fdnum);
close_file(gd);
}


int mywrite(int fd, char *buf, int nbytes)
{

int count = 0,blk;
int remain = 0;
OFT *oftp;
char wbuf[BLKSIZE];
oftp = running->fd[fd];
MINODE *mip = oftp->mptr;
INODE *ip = &mip->INODE;
char *cq = buf;
while(nbytes > 0) //As long as the data we are trying to write from the buffer to the file is literally anything more than zero , we go on
	{
	//compute LOGICAL BLOCK (lbk) and the startByte in that lbk:
	int lbk = oftp->offset / BLKSIZE;
	int startByte = oftp->offset % BLKSIZE;
	// writing in direct blocks
	if (lbk < 12)
		{
		if (ip->i_block[lbk] == 0) //This means that there is no data block yet
			{
			ip->i_block[lbk] = balloc(mip->dev); //Then we allocate a block to it 
			}
		blk = ip->i_block[lbk]; //blk should be a diskblock now
		}
	// Now we are dealing with writing to indirect block
	else if (lbk >= 12 && lbk < 256 + 12)
		{
		//indirect blocks
		//Load indirect blocks into ibuf
		int ibuf[256];
		//Allocate a new block if necessary
		if (ip->i_block[12] == 0) 
			{
			ip->i_block[12] = balloc(mip->dev);
			}
		get_block(mip->dev, ip->i_block[12], ibuf);//Allocate a new block if necessary and get the iblock into a ibuf
		if (ibuf[lbk-12] == 0) //Now we try to get the block no out 
			{
			ibuf[lbk-12] = balloc(mip->dev);
			}//record iblock 12
		//Grab indirect based on lbk
		blk = ibuf[lbk-12];
		}
	// double indirect blocks
	else
		{
		//double indirect blocks
		//Load double indirect blocks into buf
		int ibuf[256], dibuf[256];
		//Allocate a new block if necessary
		if (ip->i_block[13] == 0)
			{
			ip->i_block[13] = balloc(mip->dev);
			}
		get_block(mip->dev, ip->i_block[13], dibuf);
		//Load corresponding indirect block, diblk (double indirect we are on) - iblk (indirect block we are on)
		int dilbk = (lbk-12-256) / 256;
		int ilbk = (lbk-12-256) % 256;
		//Allocate a new block if necessary
		if (dibuf[dilbk] == 0)
			{
			dibuf[dilbk] = balloc(mip->dev);
			}
		//load in ibuf
		get_block(mip->dev, dibuf[dilbk], ibuf);
		//Allocate a new block if necessary
		if (ibuf[ilbk] == 0)
			{
			ibuf[ilbk] = balloc(mip->dev);
			}
		//set blk value
		blk = ibuf[ilbk];
		}

	get_block(mip->dev, blk, wbuf); // read disk block into wbuf[ ]
	char* cp = wbuf + startByte; // cp points at startByte in wbuf[]
	remain = BLKSIZE - startByte; //The remain is basically the number of BYTEs remain in this block
	while (remain > 0)
		{
		// write as much as remain allows
		count++;
		*cp++ = *cq++;
		// cq points at buf[ ]
		nbytes--; remain--; // dec counts (until nbytes gets to zero)
		oftp->offset++;// advance offset
		if (oftp->offset > mip->INODE.i_size) // especially for RW|APPEND mode
		mip->INODE.i_size++; // inc file size (if offset > fileSize)
		if (nbytes <= 0)
		 break; // if already nbytes, break
		}
	put_block(mip->dev, blk, wbuf); // write wbuf[ ] to disk
	}
mip->dirty = 1;
printf("wrote %d chars into fd = %d \n", count , fd);
return count;
}
