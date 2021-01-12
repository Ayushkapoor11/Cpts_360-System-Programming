/*********** util.c file ****************/

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];



//Used to grab block of data from the device
int get_block(int dev, int blk, char *buf)
{
	//Get to the specified block
	lseek(dev, (long)blk*BLKSIZE, 0);
	//Read the specified block into buf
	read(dev, buf, BLKSIZE);
}
int put_block(int dev, int blk, char *buf)
{
	//Same as above but write to the provided block using buf
	lseek(dev, (long)blk*BLKSIZE, 0);
	write(dev, buf, BLKSIZE);
} 
//Tokenizing pathname into name and setting n
int tokenize(char *line)
{
	// tokenize pathname in GLOBAL gpath[]; pointer by name[i]; n tokens
	int i = 0;
	char *token = strtok(line, "/");
	while(token != NULL)
	{
	name[i] = token;
	token = strtok(NULL, "/");
	i++;
	}
name[i] = 0;
n = i;
return 0;
}
//Return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
int i;
MINODE *mip;
char buf[BLKSIZE];
int blk, disp;
INODE *ip;
for (i=0; i<NMINODE; i++)
{
	mip = &minode[i];
	//This will not be true when usinig function for root or if inode to get does not exist (dev & ino)
	if (mip->refCount && mip->dev == dev && mip->ino == ino)
	{
		mip->refCount++;
		//printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
		return mip;
	}
}
//This for the case of createing root or when an inode with dev & ino passed does not exist
for (i=0; i<NMINODE; i++)
{
	mip = &minode[i];//Checking that this is an inode that has actually been initialized
	if (mip->refCount == 0)
	{
		//printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
		//Initializing ref count of inode to 1 and initializing dev and ino based on values passed
		mip->refCount = 1;
		mip->dev = dev;
		mip->ino = ino;
		//Properly set block and displacement for the ino (ex. 2 would be 2nd inode)
		// get INODE of ino to buf
		blk = (ino-1) / 8 + inode_start;
		disp = (ino-1) % 8;
		//printf("iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);
		//Reading inode pointer from dev and initializing it inside of minode
		get_block(dev, blk, buf);
		ip = (INODE *)buf + disp;
		// copy INODE to mp->INODE
		mip->INODE = *ip;
		return mip;
	}
}
printf("PANIC: no more free minodes\n");
return 0;
}

int iput(MINODE *mip)
{
	int i, block, offset;
	char buf[BLKSIZE];
	INODE *ip;
	//If the mip is 0 then just return as cannot add to array of minodes
	if (mip==0)
	return;
	//Why do we decrease ref count here? - to show this user done with it
	mip->refCount--;
	if (mip->refCount > 0) return;
	if (!mip->dirty)
	return;
	/* write back */
	printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino);
	block = ((mip->ino - 1) / 8) + inode_start;
	offset = (mip->ino - 1) % 8;
	/* first get the block containing this inode */
	get_block(mip->dev, block, buf);
	ip = (INODE *)buf + offset;
	*ip = mip->INODE;put_block(mip->dev, block, buf);
}
//Takes MINODE to see if the name is a child in the directory, return 0 if not
int search(MINODE *mip, char *name)
{
	//Used to hold the names and test them
	char sbuf[1024], temp[256];
	char *cp;
	//Set ip as the actual inode pointer associated with MINODE
	INODE *ip = &(mip->INODE);
	fprintf(stderr,"name %s\n",name);
	for (int i=0; i < 12; i++)
	{ // assume DIR at most 12 direct blocks
		printf("In for loop\n");
		//mip = iget(dev, ino);
		//checking if this block is free, end loop no more children
		if (ip->i_block[i] == 0)
		{
		break;
		}
	// YOU SHOULD print i_block[i] number here
	printf("In Search, IP block number:%d \n",i);
	//Read the current block into buf if it is not 0
	get_block(dev,ip->i_block[i], sbuf);
	//Create a directory and char pointer based off what was read into buf(from inode block)
	dp = (DIR *)sbuf;
	cp = sbuf;
	//Using char pointer to search the current direct block of this dir inode
	while(cp < sbuf + 1024)
	{
		//Copy the name and its length from the directory pointer
		strncpy(temp, dp->name, dp->name_len);
		//Set the end of the name to 0 or null
		temp[dp->name_len] = 0;
		//If this directory name equals the child then it is found
		if(strcmp(temp,name)==0)
		{
			fprintf(stderr,"%s found! \n",temp);
			//Return the inode of this directory pointer
			return dp->inode;
		}
		//This is incrmenting cp to the next record of this directory pointer
		cp += dp->rec_len;
		//Update the directory pointer based of the new cp
		dp = (DIR *)cp;
	}
	}
	//The child was not found in any of the direct blocks of this inode so return 0
	fprintf(stderr,"%s not found! \n",temp);
	return 0;}
	//Returning ino number (one of the 64 inodes) for this pathname, return 0 if does not exist
	int getino(char *pathname)
	{
		int i, ino, blk, disp;
		INODE *ip;
		MINODE *mip;
		printf("getino: pathname=%s\n", pathname);
		//If the case is we are searching for root just return root, root always = ino 2
		if (strcmp(pathname, "/")==0)
		return 2;
		//Now we check if this is an absolute path
		if (pathname[0]=='/')
		//If it is absolute set mip equal to the root MINODE
		mip = iget(dev, 2);
		else
		//Otherwise set mip equal to the cwd MINODE
		mip = iget(running->cwd->dev, running->cwd->ino);
		//This tokenizes the pathname with / and stores the tokens in name, n is global number of tokens
		tokenize(pathname);
		//Looping through the tokens
		for (i=0; i<n; i++)
		{
		printf("===========================================\n");
		//Gets the inode number for the next inode in the path
		ino = search(mip, name[i]);
		//Case when path is invalid one of the names do not exist
		if (ino==0)
			{
			iput(mip);
			printf("name %s does not exist\n", name[i]);
			return 0;
			}
		//if ino is valid update mip to this child
		iput(mip);
		mip = iget(dev, ino);
		//WHY do we use iput in both of these cases? - to update disk
		}
		iput(mip);
		//return the inode number of the last dir it ended on correctly
		return ino;
	}
	int tst_bit(char *buf, int bit)
	{
		int i, j;
		i = bit/8; j=bit%8;
		if (buf[i] & (1 << j))
		return 1;return 0;
	}
	int set_bit(char *buf, int bit)
	{
	int i, j;
	i = bit/8; j=bit%8;
	buf[i] |= (1 << j);
	}
	int clr_bit(char *buf, int bit)
	{
		int i, j;
		i = bit/8; j=bit%8;
		buf[i] &= ~(1 << j);
	}
	int incFreeInodes(int dev)
	{
		char buf[BLKSIZE];
		// inc free inodes count in SUPER and GD
		get_block(dev, 1, buf);
		sp = (SUPER *)buf;
		sp->s_free_inodes_count++;
		put_block(dev, 1, buf);
		get_block(dev, 2, buf);
		gp = (GD *)buf;
		gp->bg_free_inodes_count++;
		put_block(dev, 2, buf);
	}
int decFreeInodes(int dev)
{
	char buf[BLKSIZE];
	// dec free inodes count by 1 in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count--;
	put_block(dev, 1, buf);
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	put_block(dev, 2, buf);
}
int decFreeBlocks(int dev)
{
	char buf[BLKSIZE];
	// dec free inodes count by 1 in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count--;
	put_block(dev, 1, buf);get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count--;
	put_block(dev, 2, buf);
}
int incFreeBlocks(int dev)
{
	char buf[BLKSIZE];
	// dec free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count++;
	put_block(dev, 1, buf);
	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count++;
	put_block(dev, 2, buf);
}
int ialloc(int dev) // allocate an inode number
{
	int i;
	char buf[BLKSIZE];
	// read inode_bitmap block
	get_block(dev, imap, buf);
	for (i=0; i < ninodes; i++)
	{
		if (tst_bit(buf, i)==0)
		{
			set_bit(buf,i);
			put_block(dev, imap, buf);
			decFreeInodes(dev);
		return i+1;
		}
	}
return 0;
}
// SAME AS IALLOC BUT FOR BLOCKS
int balloc(int dev)
{
	int i;
	char buf[BLKSIZE];
	// read block bitmap into buf
	get_block(dev, bmap, buf);
	for(i = 0; i < nblocks; i++)
	{
		// allocate a free block
		if(tst_bit(buf, i)==0)
		{
			set_bit(buf, i);
			decFreeBlocks(dev);put_block(dev, bmap, buf);
			// return i which is its block number
			return i;
		}
	}
	return 0;
}
int idalloc(int dev, int ino) // deallocate an ino number

{
	int i;
	char buf[BLKSIZE];
	if (ino > ninodes)
	{
		printf("inumber %d out of range\n", ino);
		return;
	}
	// get inode bitmap block
	get_block(dev, bmap, buf);
	clr_bit(buf, ino-1);
	// write buf back
	put_block(dev, imap, buf);
	// update free inode count in SUPER and GD
	incFreeInodes(dev);
}

int bdalloc(int dev, int blk) // deallocate a blk number
{
	int i;
	char buf[BLKSIZE];
	if (blk > nblocks)
	{
		printf("blknumber %d out of range\n", blk);
		return;
	}
	// get inode bitmap block
	get_block(dev, bmap, buf);
	clr_bit(buf, blk-1);
	// write buf back
	put_block(dev, bmap, buf);
	// update free block count in SUPER and GD
	incFreeBlocks(dev);
}
//Add the file or directory to the first available block with space, create new block in parent dir if necessary
int enter_name(MINODE *pip, int myino, char *myname)
{
	int ideal_len, remain;
	char buf[BLKSIZE];
	DIR *dp;
	char *cp;//Calculate length of the name
	MINODE *parent = pip;
	int n_len = strlen(myname);
	//Calculate the lenght we need based on name
	int need_length = 4 * ((8 + n_len + 3) / 4); // a multiple of 4
	//Search through blocks of the inode
	for(int i = 0; i < 12; i++)
	{
		//If block is not allocated stop
		if(parent->INODE.i_block[i] == 0)
		{
		return -1;
		}
	// parents ith block
	get_block(parent->dev, parent->INODE.i_block[i], buf);
	//Set the dp and cp to look through this blocks
	dp = (DIR *)buf;
	cp = buf;
	//Increment through the directories in this block
	while(cp + dp->rec_len < buf + BLKSIZE)
	{
		cp+= dp->rec_len;
		dp = (DIR *)cp;
	}
	//Set ideal length based on final dir
	ideal_len = 4 * ((8 + dp->name_len + 3) / 4);
	//Get remaining length in the block
	remain = dp->rec_len - ideal_len;
	//If enough room is left in the block the dir can be added
	if(remain >= need_length)
	{
		//enter the new entry as the LAST entry and trim the previous entry
		//to its IDEAL_LENGTH;
		//Trim previous dir, current dp to ideal length
		dp->rec_len = ideal_len;
		//Increment dp and cp for the new dir
		cp += dp->rec_len;
		dp = (DIR *)cp;
		//Add the new dir or file
		dp->rec_len = remain;
		dp->inode = myino;
		dp->name_len = n_len;
		strncpy(dp->name, myname, n_len);
	}
	else
	{
		//Need to create a new block to store dir or file
		//Allocate a new data blockint bno = balloc(parent->dev);
		//Increase inode size of the parentquit
		parent->INODE.i_size += 1024;
		//Set the next block to this new block, if i is 12 no more direct blocks
		if(i < 11)
		{
			//parent->INODE.i_block[i+1] = bno;
		}
			//Get the new block into buf
			get_block(parent->dev, parent->INODE.i_block[i+1], buf);
			dp = (DIR *)buf;
			//Add the new dir or file
			dp->rec_len = BLKSIZE;
			dp->inode = myino;
			dp->name_len = n_len;
			strncpy(dp->name, myname, n_len);
	}
	// write block back to disk
	put_block(parent->dev, parent->INODE.i_block[i], buf);
}
}
int truncate(MINODE *mip)
{
	//Go through the direct blocks and deallocate
	for (int i = 0; i < 12; i++)
	{
		// If block is already free, go on to next iteration
		if (mip->INODE.i_block[i] == 0) 
		{
			continue;
		}
		//Deallocate this block
		bdalloc(mip->dev, mip->INODE.i_block[i]);
	}
	//Deallocate the indirect blocks if necessary
	if(mip->INODE.i_block[12] != 0)
	{
		int ibuf[256];
		get_block(mip->dev, mip->INODE.i_block[12], ibuf);
		// indirect has 256 blocks
		for(int i = 0; i < 256; i++)
		{
			//If block is empty continue
			if(ibuf[i] == 0)
			continue;
			//If it is not empty deallocate a block
			bdalloc(mip->dev, ibuf[i]);
		}
	}
	//Deallocate double indirect blocks if necessary
	if(mip->INODE.i_block[13] != 0) {
	int ibuf[256];
	int tempBuf[256];
	get_block(mip->dev, mip->INODE.i_block[13], ibuf);
	//256 indirect blocks
	for(int i = 0; i < 256; i++) 
	{
		if(ibuf[i] != 0)
		{
			get_block(mip->dev, ibuf[i], tempBuf);
			//Each 256 indirect has 256 double indirect blocks
			for(int j = 0; j < 256; j++)
			 {
				if(tempBuf[j] == 0)
				continue;
				bdalloc(mip->dev, tempBuf[j]);
			 }
		bdalloc(mip->dev, ibuf[i]);
		}
	}
}
// touch mips time and set it dirty
mip->INODE.i_atime = time(0L);
mip->INODE.i_mtime = time(0L);
mip->INODE.i_ctime = time(0L);
mip->INODE.i_size = 0;
mip->dirty = 1;
}
