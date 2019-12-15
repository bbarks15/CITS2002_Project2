#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#include "sifs-internal.h"
#include "sifs-helper.h"

int SIFS_rmdir(const char *volumename, const char *pathname)
{

    // SPLIT PATH
    char *filepath[SIFS_MAX_ENTRIES];
    int dirdepth;
    getPath(pathname, filepath, &dirdepth);

    // OPEN VOLUME
    FILE *vol = fopen(volumename, "r+");

    // CHECK INVALID ARGUMENT
    if (volumename == NULL || pathname == NULL) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    // CHECK VOLUME EXISTS
    if(access(volumename, F_OK)!= 0) {
       SIFS_errno = SIFS_ENOVOL;
       return 1;
    }

    // CHECK IF VOLUME
    if(isVol(volumename, vol) != 0) {
        SIFS_errno = SIFS_ENOTVOL;
        return 1;
    }

    // memory allocation failed SIFS_ENOMEM

    // GET HEADER
    SIFS_VOLUME_HEADER header;
    fseek(vol, 0, SEEK_SET);
    fread(&header, sizeof(SIFS_VOLUME_HEADER), 1, vol);

    // GET BITMAP
    uint32_t nblocks = header.nblocks;
    SIFS_BIT bitmap[nblocks];
    memset(bitmap, 0, sizeof(bitmap));
    fread(&bitmap, sizeof(SIFS_BIT), nblocks, vol);

    //FIND PARENT DIR
    SIFS_DIRBLOCK parentdir;
    SIFS_BLOCKID parentdir_blockid = 0;
    SIFS_BLOCKID dir_block_id;
    int dir_entry;
    int current_depth = 0;
    memset(&parentdir, 0, sizeof(SIFS_DIRBLOCK));
    fread(&parentdir, sizeof(SIFS_DIRBLOCK), 1, vol);

    if (dirdepth<2) {
        findDir(volumename,filepath[dirdepth],parentdir,&dir_block_id,&dir_entry);
    }
    else {
        while (strcmp(parentdir.name,filepath[dirdepth - 1]) != 0
                && current_depth <= dirdepth) {
            findDir(volumename,filepath[current_depth++],parentdir,
                        &parentdir_blockid,&dir_entry);
            fseek(vol, PARENT_POS, SEEK_SET);
            memset(&parentdir, 0, sizeof(SIFS_DIRBLOCK));
            fread(&parentdir, sizeof(SIFS_DIRBLOCK), 1, vol);
        }
        findDir(volumename, filepath[dirdepth], parentdir, &dir_block_id, 
                    &dir_entry);
    }

    // CHECK IF DIR
    if (bitmap[dir_block_id] != SIFS_DIR) {
        SIFS_errno = SIFS_ENOTDIR;
        return 1;
    }

    // CHECK IF EXISTS
    if (entryExists(vol, parentdir, filepath[dirdepth])!=0) {
       SIFS_errno = SIFS_ENOENT;
       return 1;
    }

    // CHECK IF DIR IS EMPTY
    SIFS_DIRBLOCK dir;
    fseek(vol, NEW_POS, SEEK_SET);
    fread(&dir, sizeof(SIFS_DIRBLOCK), 1, vol);
    if (dir.nentries != 0) {
       SIFS_errno = SIFS_ENOTEMPTY;
       return 1;
    }

    // WRITE BITMAP
    bitmap[dir_block_id] = SIFS_UNUSED;
    fseek(vol, HEADER_SIZE, SEEK_SET);
    fwrite(&bitmap, sizeof(bitmap), 1, vol);

    // UPDATE PARENT
    parentdir.modtime = time(NULL);
    for (int i = dir_entry; i < parentdir.nentries; i++) {
        memcpy(&parentdir.entries[i], &parentdir.entries[i+1], 
                sizeof(parentdir.entries[i+1]));
    }
    parentdir.nentries--;

    // WRITE PARENT
    fseek(vol, PARENT_POS, SEEK_SET);
    fwrite(&parentdir, sizeof parentdir, 1, vol);

    // REMOVE DIR
    fseek(vol, NEW_POS, SEEK_SET);
    memset(&parentdir, 0, sizeof(SIFS_DIRBLOCK));
    fwrite(&parentdir, sizeof parentdir, 1, vol);

    // CLOSE VOL
    fclose(vol);

    // RETURN 0 SUCCESS!
    SIFS_errno	= SIFS_EOK;
    return 0;
}
