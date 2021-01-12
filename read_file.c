extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256], newfile[256];

int myread(int fd, char *buf, int nbytes)
{
int count = 0,blk;
int remain = 0;
OFT *oftp;
char readbuf[BLKSIZE];
oftp = running->fd[fd];
MINODE *mip = oftp->mptr;
INODE *ip = &mip->INODE;
int fileSize = ip->i_size;
//number of bytes still available for read
int avil = fileSize - running->fd[fd]->offset;
//cq points at buf[ ]
char *cq = buf;
while (nbytes && avil)
{
	//Compute LOGICAL BLOCK number lbk and startByte in that block from offset;
	int lbk
	= oftp->offset / BLKSIZE;
	int startByte = oftp->offset % BLKSIZE;
	// I only show how to read DIRECT BLOCKS. YOU do INDIRECT and D_INDIRECT
	if (lbk < 12)
		{
			// lbk is a direct block
			blk = ip->i_block[lbk]; // map LOGICAL lbk to PHYSICAL blk
		}
	else if (lbk >= 12 && lbk < 256 + 12) 
		{
		//indirect blocks
		//Load indirect blocks into ibuf
		int ibuf[256];
		get_block(mip->dev, ip->i_block[12], ibuf);
		//Grab indirect based on lbk
		blk = ibuf[lbk-12];
		}
	else
		{
		//double indirect blocks
		//Load double indirect blocks into buf
		int ibuf[256], dibuf[256];
		get_block(mip->dev, ip->i_block[13], dibuf);
		//Load corresponding indirect block, diblk (double indirect we are on) - iblk (indirect block we are on)
		int dilbk = (lbk-12-256) / 256;
		int ilbk = (lbk-12-256) % 256;//load in ibuf
		get_block(mip->dev, dibuf[dilbk], ibuf);
		//set blk value
		blk = ibuf[ilbk];
		}
	/* get the data block into readbuf[BLKSIZE] */
	get_block(mip->dev, blk, readbuf);
	//printf("\nreadbuf: %s", readbuf);
	/* copy from startByte to buf[ ], at most remain bytes in this block */
	char *cp = readbuf + startByte;
	remain = BLKSIZE - startByte; // number of bytes remain in readbuf[]
	while (remain > 0)
	{
		*cq++ = *cp++;
		// copy byte from readbuf[] into buf[]
		oftp->offset++;
		// advance offset
		count++;
		// inc count as number of bytes read
		avil--; nbytes--; remain--;
		if (nbytes <= 0 || avil <= 0)
		break;
	}
	// if one data block is not enough, loop back to OUTER while for more ...
}
//printf("myread: read %d char from file descriptor %d\n", count, fd);
return count; // count is the actual number of bytes read
}
int mycat()
{
	char mybuf[1024], dummy = 0; // a null char at end of mybuf[ ]
	int n;
	//Set newfile to read for the open function
	strcpy(newfile,"0");
	//open the file and return the index in fd for proc
	int fdnum = open_file();
	//file doesnt exist
	if (fdnum == -1)
	{
		printf("Invalid path, no such files exist \n");
		return -1;
	}
	printf("\nStart of file: \n");
	//reading
	int i = 0;
	n = myread(fdnum, mybuf, BLKSIZE);
	while(n > 0)
	{
		mybuf[n] = 0;
		while(i<n)
		{
			printf("%c", mybuf[i]);
			i++;
		}
		i = 0;
		n = myread(fdnum, mybuf, BLKSIZE);
	}	
	printf("\nEnd of file \n");
	//Close the file
	close_file(fdnum);
}
