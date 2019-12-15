#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "sifs-internal.h"
#include "sifs-helper.h"

// remove an existing file from an existing volume
int SIFS_rmfile(const char *volumename, const char *pathname)
{
    // IF ARGUMENTS ARE VALID
    if (volumename == NULL || pathname == NULL) {
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
    
    // SPLIT PATH INTO STRING ARRAY
    char *filepath[SIFS_MAX_ENTRIES];
    int filedepth;
    getPath(pathname, filepath, &filedepth);

    // GET HEADER AND BITMAP
    SIFS_VOLUME_HEADER header;
    fseek(vol, 0, SEEK_SET);
    memset(&header, 0, sizeof(SIFS_VOLUME_HEADER));
    fread(&header, sizeof(SIFS_VOLUME_HEADER), 1, vol);
    uint32_t nblocks = header.nblocks;
    size_t blocksize = header.blocksize;

    SIFS_BIT bitmap[nblocks];
    memset(&bitmap, 0, sizeof(bitmap));
    fread(&bitmap, sizeof(SIFS_BIT), nblocks, vol);

    // LOOK FOR PARENT DIR OF FILENAME
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

            // FIND AND SET PARENT DIR
            findDir(volumename,filepath[currentdepth++],parentdir,
                &parentdirblockid,&direntry);

            fseek(vol, ROOT_POS + (parentdirblockid * blocksize),
                    SEEK_SET);
            memset(&parentdir, 0, sizeof(SIFS_DIRBLOCK));
            fread(&parentdir, sizeof(SIFS_DIRBLOCK), 1, vol);

        }

    }

    // CHECK IF FILE EXISTS
    if(entryExists(vol, parentdir, filepath[filedepth]) != 0) {
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }

    // GET THE CHILD
    SIFS_FILEBLOCK childfile;
    uint32_t childnameindex;
    uint32_t childentryid;
    uint32_t childfileblockid; 

    for (childentryid = 0; childentryid < parentdir.nentries; ++childentryid) {

        // LOOK AT ALL ENTRIES IN PARENT
        childnameindex = parentdir.entries[childentryid].fileindex;
        childfileblockid = parentdir.entries[childentryid].blockID;

        fseek(vol, 
            ROOT_POS + (childfileblockid * header.blocksize), 
            SEEK_SET);
        memset(&childfile, 0, sizeof(SIFS_FILEBLOCK));
        fread(&childfile, sizeof(SIFS_DIRBLOCK), 1, vol);

        // IF FILE NAME MATCHES, CHILD IS FOUND
        if(strcmp(childfile.filenames[childnameindex], filepath[filedepth]) == 0) {
            break;
        }

    }

    // CHECK IF CHILD IS FILE
    if (bitmap[childfileblockid] != SIFS_FILE) {
        SIFS_errno = SIFS_ENOTFILE;
        return 1;
    }

    if(childfile.nfiles == 1) {

        // IF THERE IS ONLY ONE FILE, WE CAN DELETE THE BLOCK COMPLETELY
    	for (int i = childfile.firstblockID; i < childfile.firstblockID + 
    			(childfile.length + blocksize - 1) / blocksize; ++i) {
    		bitmap[i] = SIFS_UNUSED;
    	}

    	memset(&childfile, 0, sizeof(SIFS_FILEBLOCK));
    	bitmap[childfileblockid] = SIFS_UNUSED;
    }
    else {

        // OTHERWISE, REMOVE INSTANCES OF THAT ENTRY IN THE FILE BLOCK
    	memset(&childfile.filenames[childnameindex], 0, SIFS_MAX_NAME_LENGTH);

    	if(childnameindex != childfile.nfiles - 1) {

    		for (int i = childnameindex + 1; i < childfile.nfiles; ++i) {
		    	strcpy(childfile.filenames[i-1], childfile.filenames[i]);
    		}

    		memset(&childfile.filenames[childfile.nfiles-1], 0, 
    			SIFS_MAX_NAME_LENGTH);
    	}

    	childfile.nfiles--;
    	childfile.modtime = time(NULL);
    }

    // WRITE BITMAP
    fseek(vol, HEADER_SIZE, SEEK_SET);
    fwrite(&bitmap, BITMAP_SIZE, 1, vol);

    // WRITE CHILD FILE
    fseek(vol, ROOT_POS + (childfileblockid * blocksize), SEEK_SET);
    fwrite(&childfile, sizeof(SIFS_FILEBLOCK), 1, vol);

    // UPDATE PARENT METADATA AND FILE INDEX IN PARENT ENTRIES
    for (int i = childentryid; i < parentdir.nentries; i++) {
    	if(parentdir.entries[i+1].blockID == childfileblockid) {
    		parentdir.entries[i+1].fileindex--;
    	}
        memcpy(&parentdir.entries[i], &parentdir.entries[i+1], 
        	sizeof(parentdir.entries[i+1]));
    }

    parentdir.nentries--;
    parentdir.modtime = time(NULL);

    // WRITE PARENT
    fseek(vol, ROOT_POS + (parentdirblockid * blocksize), SEEK_SET);
    fwrite(&parentdir, sizeof(SIFS_DIRBLOCK), 1, vol);

    fclose(vol);

    SIFS_errno	= SIFS_EOK;
    return 0;
}
