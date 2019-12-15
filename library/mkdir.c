#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#include "sifs-internal.h"
#include "sifs-helper.h"


int SIFS_mkdir(const char *volumename, const char *pathname)
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

    // OPEN VOLUME
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

    // SPLIT PATH INTO STRING ARRAY
    char *filepath[header.nblocks];
    int dirdepth;
    getPath(pathname, filepath, &dirdepth);

    // CHECK NAME LENGTH
    if(strlen(filepath[dirdepth]) > SIFS_MAX_NAME_LENGTH) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    // MAKE NEW DIR
    SIFS_DIRBLOCK newdir;
    memset(&newdir, 0, sizeof(newdir));
    strcpy(newdir.name, filepath[dirdepth]);
    newdir.modtime = time(NULL);
    newdir.nentries = 0;

    // GET BITMAP
    uint32_t nblocks = header.nblocks;
    SIFS_BIT bitmap[nblocks];
    memset(&bitmap, 0, sizeof(bitmap));
    fread(&bitmap,sizeof(SIFS_BIT),nblocks, vol);

    // GET FIRST UNUSED BLOCK
    uint32_t dir_block_id = 0;
    for (int i = 0; i < nblocks ; i++) {
        if (bitmap[i] == SIFS_UNUSED ) {
            dir_block_id = i;
            break;
        }
    }

    // CHECK SPACE ON VOLUME
    if (dir_block_id == 0) {
        SIFS_errno=SIFS_ENOSPC;
        return 1;
    }

    // UPDATE BITMAP
    bitmap[dir_block_id] = SIFS_DIR;

    
    // GET PARENT DIR
    SIFS_DIRBLOCK parentdir;
    SIFS_BLOCKID parentdir_blockid = 0;
    int dir_entry;
    int current_depth = 0;

    // GET ROOT DIR
    memset(&parentdir, 0, sizeof(SIFS_DIRBLOCK) );
    fread(&parentdir, sizeof(SIFS_DIRBLOCK), 1, vol);

    if (dirdepth > 1) {
        while(strcmp(parentdir.name,filepath[dirdepth-1]) != 0
                && current_depth <= dirdepth) {
            findDir(volumename,filepath[current_depth++],parentdir,
                    &parentdir_blockid,&dir_entry);
            fseek(vol, PARENT_POS, SEEK_SET);
            memset(&parentdir, 0, sizeof(SIFS_DIRBLOCK));
            fread(&parentdir, sizeof(SIFS_DIRBLOCK), 1, vol);
        }
    }

    // CHECK IF DIR EXISTS
    if(entryExists(vol, parentdir, filepath[dirdepth]) == 0) {
        SIFS_errno = SIFS_EEXIST;
        return 1;
    }

    // CHECK DIR/FILE ENTRIES
    if (parentdir.nentries >= 24) {
        SIFS_errno = SIFS_EMAXENTRY;
        return 1;
    }

    // WRITE NEW BITMAP
    fseek(vol, HEADER_SIZE, SEEK_SET);
    fwrite(&bitmap, sizeof bitmap, 1, vol);

    // WRITE PARENT DIR
    parentdir.entries[parentdir.nentries].blockID = dir_block_id;
    parentdir.nentries++;
    parentdir.modtime = time(NULL);
    fseek(vol, PARENT_POS, SEEK_SET);
    fwrite(&parentdir, sizeof parentdir, 1, vol);

    // WRITE NEW DIR
    char block[header.blocksize];
    memset(block, 0, sizeof block);
    memcpy(block, &newdir, sizeof newdir);
    fseek(vol, NEW_POS, SEEK_SET);
    fwrite(block, sizeof newdir, 1, vol);

    // CLOSE VOL
    fclose(vol);

    // RETURN 0 SUCCESS!
    SIFS_errno  = SIFS_EOK;
    return 0;
}

