#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "sifs-internal.h"
#include "sifs-helper.h"

// read the contents of an existing file from an existing volume
int SIFS_readfile(const char *volumename, const char *pathname,
          void **data, size_t *nbytes)
{

    // IF ARGUMENTS ARE VALID
    if (volumename == NULL || pathname == NULL || data == NULL) {
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

            // FIND AND SET PARENT OF FILE
            findDir(volumename,filepath[currentdepth++],parentdir,
                &parentdirblockid,&direntry);

            fseek(vol, ROOT_POS + (parentdirblockid * blocksize),
                    SEEK_SET);
            memset(&parentdir, 0, sizeof(SIFS_DIRBLOCK));
            fread(&parentdir, sizeof(SIFS_DIRBLOCK), 1, vol);
        }

    }

    // CHECK IF ENTRY EXISTS
    if(entryExists(vol, parentdir, filepath[filedepth]) != 0) {
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }

    // GET THE CHILD
    SIFS_FILEBLOCK childfile;
    uint32_t childindex;
	uint32_t childfileblockid;

    for (int i = 0; i < parentdir.nentries; ++i) {

        childindex = parentdir.entries[i].fileindex;
		childfileblockid = parentdir.entries[i].blockID;

        fseek(vol,
            ROOT_POS + (parentdir.entries[i].blockID * header.blocksize),
            SEEK_SET);
        memset(&childfile, 0, sizeof(SIFS_FILEBLOCK));
        fread(&childfile, sizeof(SIFS_DIRBLOCK), 1, vol);

        // LOOK FOR THE FILE IN PARENT AND UPDATE DATA AND NBYTES
        if(strcmp(childfile.filenames[childindex], filepath[filedepth]) == 0) {
            break;
        }
    }

    // CHECK IF CHILD IS FILE
    if (bitmap[childfileblockid] != SIFS_FILE) {
        SIFS_errno = SIFS_ENOTFILE;
        return 1;
    }

    char mychar[childfile.length];

    fseek(vol,
        ROOT_POS + (childfile.firstblockID * header.blocksize),
        SEEK_SET);
    fread(&mychar, childfile.length, 1, vol);
            
    *nbytes = childfile.length;

	mychar[childfile.length] = '\0';

    *data = calloc(childfile.length + 1, sizeof(char));
    *data = &mychar;

    fclose(vol);

    SIFS_errno  = SIFS_EOK;
    return 0;
}
