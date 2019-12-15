#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "sifs-internal.h"
#include "sifs-helper.h"

// get information about a requested directory
int SIFS_dirinfo(const char *volumename, const char *pathname,
                 char ***entrynames, uint32_t *nentries, time_t *modtime)
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

    // OPEN FILE
    FILE *vol = fopen(volumename, "r");

    // CHECK IF VOL IS A VOLUME
    if(isVol(volumename, vol) != 0) {
        SIFS_errno = SIFS_ENOTVOL;
        return 1;
    }

    // Memory Allocation Failed SIFS_ENOMEM

    // SPLIT PATH
    char *filepath[SIFS_MAX_ENTRIES];
    int dirdepth;
    getPath(pathname, filepath, &dirdepth);

    // GET HEADER
    SIFS_VOLUME_HEADER header;
    fseek(vol, 0, SEEK_SET);
    fread(&header, sizeof(SIFS_VOLUME_HEADER), 1, vol);

    // GET BITMAP
    uint32_t nblocks = header.nblocks;
    SIFS_BIT bitmap[nblocks];
    memset(bitmap, 0, sizeof(bitmap));
    fread(&bitmap, sizeof(SIFS_BIT), nblocks, vol);

    // FIND DIR
    SIFS_DIRBLOCK dir;
    SIFS_BLOCKID dir_block_id = 0;
    int dir_entry;

    int current_depth = 1;

    fseek(vol, ROOT_POS, SEEK_SET);
    memset(&dir, 0, sizeof(SIFS_DIRBLOCK));
    fread(&dir, sizeof(SIFS_DIRBLOCK), 1, vol); // get root dir

    // FIND PARENT
    if (dirdepth == 1) {
        findDir(volumename,filepath[current_depth],dir,&dir_block_id,&dir_entry);
    }
    else {
        while (strcmp(dir.name,filepath[dirdepth-1]) != 0
                && current_depth <= dirdepth) {
            findDir(volumename,filepath[current_depth++],dir,&dir_block_id,
                        &dir_entry);
            fseek(vol, NEW_POS, SEEK_SET);
            memset(&dir, 0, sizeof(SIFS_DIRBLOCK));
            fread(&dir, sizeof(SIFS_DIRBLOCK), 1, vol);
        }
    }

    // CHEKC IF DIR
    if (bitmap[dir_block_id] != SIFS_DIR) {
        SIFS_errno = SIFS_ENOTDIR;
        return 1;
    }

    // CHECK IF DIR EXISTS
    if(entryExists(vol, dir, filepath[dirdepth]) != 0) {
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }

    // FIND DIR
    findDir(volumename,filepath[current_depth++],dir,&dir_block_id,&dir_entry);
    fseek(vol, NEW_POS, SEEK_SET);
    memset(&dir, 0, sizeof(SIFS_DIRBLOCK));
    fread(&dir, sizeof(SIFS_DIRBLOCK), 1, vol);

    // CHANGE VALUES TO FUNCTION
    *nentries = dir.nentries;
    *modtime = dir.modtime;
    char **entries_list = NULL;
    if (dir.nentries == 0) {
        *entrynames = entries_list;
        return 1;
    }

    //GET ENTRIES
    SIFS_DIRBLOCK tempdir;
    SIFS_BLOCKID temp_blockid;

    for (int i = 0; i < *nentries; i++) {

        temp_blockid = dir.entries[i].blockID;

        fseek(vol, TEMPNEW_POS, SEEK_SET);
        memset(&tempdir, 0, sizeof(SIFS_DIRBLOCK));
        fread(&tempdir, sizeof(SIFS_DIRBLOCK), 1, vol);

        entries_list = realloc(entries_list, (i+1)*sizeof(entries_list[0]));
        entries_list[i] = strduplicate(tempdir.name);

    }

    // CLOSE VOL
    fclose(vol);

    // ASSISGN ENTRIES TO POINTER
    *entrynames = entries_list;

    // RETURN 0 SUCCESS!
    SIFS_errno	= SIFS_EOK;
    return 0;
}

