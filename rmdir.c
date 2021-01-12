/************* rmdir.c file **************/




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

int rmdir()


{

	//This is used to hold inumber of pathname
	int ino;
	//This is used for minode of the pathname
	MINODE *mip;
	//This is used to hold dir names when iterating
	char temp[256];
	//Get inumber using pathname
	ino = getino(pathname);
	//NOw the OS makes sure that the pathname grabbed ino in a correct manner
	if(ino == 0)
	{
		printf("wrong pathname!\n");
		return -1;
	}
	//Using inumber to get it MINODE
	mip = iget(dev,ino);
	INODE *ip = &mip->INODE;
	//Check Ownership, Super User ok or uid must match
	//Check to make sure the indode is a directory
	if(!S_ISDIR(ip->i_mode))
	{
	printf("Invalid pathname, not a dir!\n");
	iput(mip);
	return -1;
	}
	//Used to hold the blocks while searching through
	char buf[BLKSIZE];
	//Used to hold pointer values in the block
	char *cp;
	DIR *dp;
	//Now the focus is to check if the directory is empty
	if(ip->i_links_count <= 2)
	{
		//Check to make sure no entries other than . and ..
		//If this is not 0 than it is not empty
		if(mip->INODE.i_block[0] != 0)
		{
		//Get the block and iterate over to see contents
		get_block(dev, mip->INODE.i_block[0], buf);
		//Set char and dir pointer to iterate over block
		cp = buf;
		dp = (DIR*)buf;
		while (cp < buf + BLKSIZE)
	 	   {
			//Copy name into temp from dir pointer
		strncpy(temp, dp->name, dp->name_len);
		temp[dp->name_len] = 0;
		//For each new dir pointer check if name is not . or ..
		if (strcmp(temp, ".") != 0 && strcmp(temp, "..") != 0)
			{
			//If temp is not one of the . or .. then it is a file and dir is not empty
			printf("Directory is not empty\n");
			iput(mip);
			return -1;
			}			
		//Increment character and directory pointer
		cp += dp->rec_len;
		dp = (DIR*)cp;
		   }		
	}

	} 
	else 
	{
		//More than 2 link count so dir is not empty
		printf("Directory is not empty\n");
		iput(mip);
		return -1;
	}
	//Now we go on to the process of removal
	//Deallocate direct blocks that are not empty
	for (int i=0; i<12; i++)
	{
		if (mip->INODE.i_block[i]==0)
		continue;
		bdalloc(mip->dev, mip->INODE.i_block[i]);
	}

	//Deallocate the inode
	idalloc(mip->dev, mip->ino);
	iput(mip); //(which clears mip->refCount = 0)//Used to hold the pathname of parent and child in provided path
	char parent[256], child[256];
	//MINODE for parent
	MINODE *pip;
	//Set the parent and child paths from pathname, to get their inumbers
	strcpy(temp, pathname);
	strcpy(parent, dirname(temp));
	strcpy(temp, pathname);
	strcpy(child, basename(temp));
	//Getting parent ino number and MINODE
	int pino = getino(parent);
	pip = iget(mip->dev,pino);
	rm_child(pip, child);
	//Decrement link count, touch atime and mtime, mark dirty
	pip->INODE.i_links_count--;
	pip->dirty = 1;
	pip->INODE.i_atime = time(0L); // set to current time	
	pip->INODE.i_mtime = time(0L);
	iput(pip);
	//the zero would be mean it has happened successfully
	return 0;
}

// rm_child(): remove the entry [INO rlen nlen name] from parent's data block.
int rm_child(MINODE *parent, char *name)
	{
	//Used to hold the blocks and dir names while searching through
	char buf2[BLKSIZE], temp2[256];
	//Used to hold pointer values in the block
 	char *cp;
 	char *prevcp;
 	DIR *dp;
 	int curr_rlen, prev_rlen;
	//Search parent inodes data blocks for the entry of name
 	for(int i = 0; i < 12; i ++)
	{
		//Check to make sure no entries other than . and ..
		//If this is not 0 than it is not empty
	 	if(parent->INODE.i_block[i] != 0)
			{
			//Get the block and iterate over to see contents
	 		get_block(dev, parent->INODE.i_block[i], buf2);
			//Set char and dir pointer to iterate over block
	 		cp = buf2;
	 		prevcp = buf2;
	 		dp = (DIR*)buf2;
	 		while (cp < buf2 + BLKSIZE)
				{
			//Copy name into temp from dir pointer
	 		strncpy(temp2, dp->name, dp->name_len);
	 		temp2[dp->name_len] = 0;//Check to see if dp is pointing to correct child
	 			if(strcmp(name,temp2) == 0)
				{
	 				//Set curr_rlen so you can add to previous dp
	 				curr_rlen = dp->rec_len;
	 				//We know dir pointer is the correct child
	 				//Check to see if this is the last dir in the block
	 				if(cp + dp->rec_len >= buf2 + BLKSIZE)
	 					{
	 					//Back up to the previous dp
						cp -= prev_rlen;
						dp = (DIR *)cp;
						//Add the current to previous rlen
						dp->rec_len += curr_rlen;
						//Write the parent back to the disk and mark dirty
						put_block(parent->dev, parent->INODE.i_block[i], buf2);
						parent->dirty = 1;
						return 0;
	 					}
					//Check to see if this is the first dir in the block
					if(dp->rec_len == BLKSIZE)
						{
						//deallocate the current block
						bdalloc(parent->dev, parent->INODE.i_block[i]);
						//Set the iblock that held its name to zero and mark as dirty
						parent->INODE.i_block[i] = 0;
						parent->dirty = 1;
						//Shift nonempty blocks upward so there are no holes
						for(int j = i; j < 11; j++)
							{
							//Checking to see if next inode is empty
							if(parent->INODE.i_block[j+1] != 0)
								{
							//If it is not empty move block up and increment
							parent->INODE.i_block[j] = parent->INODE.i_block[j+1];
							//Set this block to empty after moving
							parent->INODE.i_block[j+1] = 0;
								}
						else 
						{
						break;
						}	
				}
			return 0;
			}
			//We now know that the dir is in the middle of the block
			//Hold length of current dir to remove
  			curr_rlen = dp->rec_len;
			//Iterate through the other directories shifting to the left
   			while(cp < buf2 + BLKSIZE)
				{
				//Increment directory and cp
  				cp += dp->rec_len;
  				dp = (DIR *)cp;
				//Use memove to shift current dp to the left
  				memmove(prevcp, cp, dp->rec_len);
				//check if this is last dir
				if(cp + dp->rec_len >= BLKSIZE)
					{
  					dp = (DIR *)prevcp;
					dp->rec_len += curr_rlen;
					break;
					}
				//Increment prevcp
				prevcp += dp->rec_len;
				}
			//Write the parent block to the disk and mark dirty
			put_block(parent->dev, parent->INODE.i_block[i],buf2);
			parent->dirty = 1;
			return 0;
			}
		//Update the previous rec len
		prev_rlen = dp->rec_len;
	
		//Increment character and directory pointer
		cp += dp->rec_len;
		prevcp += dp->rec_len;
		dp = (DIR*)cp;
		}
	}
	 else 
		{
		break;
		}
	}
}
