#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "sifs-internal.h"
#include "sifs-helper.h"

// add a copy of a new file to an existing volume
int SIFS_writefile(const char *volumename, const char *pathname,
		   void *data, size_t nbytes)
{
    // IF ARGUMENTS ARE VALID
    if (volumename == NULL || pathname == NULL ||
    	data == NULL || nbytes <= 0) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    // IF VOLUME EXISTS
    if(access(volumename, F_OK)!= 0) {
       SIFS_errno = SIFS_ENOVOL;
       return 1;
    }

    FILE *vol = fopen(volumename, "r+");

    // CHECK IF VOL IS A VOLUME
    if(isVol(volumename, vol) != 0) {
        SIFS_errno = SIFS_ENOTVOL;
        return 1;
    }

    // GET HEADER
    SIFS_VOLUME_HEADER header;
    fseek(vol, 0, SEEK_SET);
    memset(&header, 0, sizeof(SIFS_VOLUME_HEADER));
    fread(&header, sizeof(SIFS_VOLUME_HEADER), 1, vol);
    uint32_t nblocks = header.nblocks;
    size_t blocksize = header.blocksize;

    // SPLIT PATH INTO STRING ARRAY
    char *filepath[nblocks];
    int filedepth;
    getPath(pathname, filepath, &filedepth);

    // CHECK NAME LENGTH
    if(strlen(filepath[filedepth]) > SIFS_MAX_NAME_LENGTH) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    // GET BITMAP
    SIFS_BIT bitmap[nblocks];
    memset(&bitmap, 0, sizeof(bitmap));
    fread(&bitmap, sizeof(SIFS_BIT), nblocks, vol);

    // CHECK IF FILENAME ALREADY EXISTS IN SUBDIR
    SIFS_DIRBLOCK parentdir;
    int currentdepth = 0;
    SIFS_BLOCKID parentdirblockid = 0;
    int direntry;

    fseek(vol, ROOT_POS, SEEK_SET);
    memset(&parentdir, 0, sizeof(SIFS_DIRBLOCK) );
    fread(&parentdir, sizeof(SIFS_DIRBLOCK), 1, vol);

    if (filedepth > 1) {
    	while(strcmp(parentdir.name, filepath[filedepth - 1]) != 0
    	        && currentdepth <= filedepth) {
    	    findDir(volumename,filepath[currentdepth++],parentdir,
    	        &parentdirblockid,&direntry);
    	    fseek(vol, ROOT_POS + (parentdirblockid * blocksize),
    	    		SEEK_SET);
    	    memset(&parentdir, 0, sizeof(SIFS_DIRBLOCK));
    	    fread(&parentdir, sizeof(SIFS_DIRBLOCK), 1, vol);
    	}
    }

    if(entryExists(vol, parentdir, filepath[filedepth]) == 0) {
        SIFS_errno = SIFS_EEXIST;
        return 1;
    }

    // CHECK DIR/FILE ENTRIES
    if (parentdir.nentries >= 24) {
        SIFS_errno = SIFS_EMAXENTRY;
        return 1;
    }

    // GET NEWFILE MD5
    unsigned char	newfileMD5[MD5_BYTELEN];
    MD5_buffer(data, nbytes, newfileMD5);
    char *newfileMD5str = malloc(sizeof(char)*MD5_STRLEN);
    memcpy(newfileMD5str, MD5_format(newfileMD5), sizeof(char) * MD5_STRLEN);

    // COMPARE MD5 OF NEW FILE TO ALL FILE BLOCKS
    SIFS_FILEBLOCK currentfile;
    char *currentfileMD5str;
    uint32_t currentfileblockid;
    int isduplicate = 0;

    for (int i = 0; i < nblocks; ++i) {
    	if(bitmap[i] == SIFS_FILE) {

    		fseek(vol, ( ROOT_POS + (i * header.blocksize) ), SEEK_SET);
    		memset(&currentfile, 0, sizeof(SIFS_FILEBLOCK));
    		fread(&currentfile, sizeof(SIFS_FILEBLOCK), 1, vol);
    		currentfileMD5str = MD5_format(currentfile.md5);

			if(strcmp(newfileMD5str, currentfileMD5str) == 0) {
				currentfile.modtime = time(NULL);
				strcpy(currentfile.filenames[currentfile.nfiles],
					filepath[filedepth]);
				currentfile.nfiles++;
				currentfileblockid = i;
				isduplicate = 1;
				break;
			}
    	}
    }

    if(isduplicate == 0) {
    	SIFS_FILEBLOCK newfile;
    	int newfileblocks = (nbytes + blocksize - 1) / blocksize;

    	uint32_t fileblockid = 0;
    	for (int i = 0; i < nblocks ; i++) {
    	    if (bitmap[i] == SIFS_UNUSED) {
    	    	bitmap[i] = SIFS_FILE;
    	        fileblockid = i;
    	        break;
    	    }
    	}

    	for (int i = 0; i < nblocks; i++) {
    		int breaknext = 0;
        	if (bitmap[i] == SIFS_UNUSED) {
        		for (int j = 1; j < newfileblocks; j++) {
                    if(header.nblocks == i + j) {
                        SIFS_errno = SIFS_ENOSPC;
                        return 1;
                    }
        			if(bitmap[i + j] != SIFS_UNUSED) {
        				breaknext = 1;
        				break;
        			}
        		}
        		if(breaknext == 0) {
        			newfile.firstblockID = i;
        	    	break;
        		}
        	}
    	}

    	if(newfile.firstblockID == 0) {
    		SIFS_errno = SIFS_ENOSPC;
        	return 1;
    	}

    	// ASSIGN VARIABLES
    	strcpy(newfile.filenames[0], filepath[filedepth]);
    	newfile.modtime = time(NULL);
    	newfile.length = nbytes;
    	memcpy(newfile.md5, newfileMD5, sizeof(newfile.md5));
    	newfile.nfiles = 1;

    	for (int i = newfile.firstblockID; i < newfile.firstblockID + newfileblocks;
	    		++i) {
    		bitmap[i] = SIFS_DATABLOCK;
	    }

    	parentdir.entries[parentdir.nentries].blockID = fileblockid;
    	parentdir.entries[parentdir.nentries].fileindex = newfile.nfiles-1;

    	fseek(vol, ROOT_POS + (newfile.firstblockID * blocksize), SEEK_SET);
    	fwrite(data, nbytes, 1, vol);

    	fseek(vol, ROOT_POS + (fileblockid * blocksize), SEEK_SET);
    	fwrite(&newfile, sizeof(SIFS_FILEBLOCK), 1, vol);

    }
    else {
    	fseek(vol, ROOT_POS + (currentfileblockid * blocksize), SEEK_SET);
    	fwrite(&currentfile, sizeof(SIFS_FILEBLOCK), 1, vol);

    	parentdir.entries[parentdir.nentries].blockID = currentfileblockid;
    	parentdir.entries[parentdir.nentries].fileindex = currentfile.nfiles-1;
    }

    parentdir.nentries++;
    parentdir.modtime = time(NULL);

    fseek(vol, HEADER_SIZE, SEEK_SET);
    fwrite(&bitmap, sizeof bitmap, 1, vol);

    fseek(vol, ROOT_POS + (parentdirblockid * blocksize), SEEK_SET);
    fwrite(&parentdir, sizeof(SIFS_DIRBLOCK), 1, vol);

    fclose(vol);

	SIFS_errno	= SIFS_EOK;
    return 0;
}


