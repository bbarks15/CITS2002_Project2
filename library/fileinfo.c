#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sifs-internal.h"
#include "sifs-helper.h"

int SIFS_fileinfo(const char *volumename, const char *pathname,
		  size_t *length, time_t *modtime)
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

    // SPLIT PATH
    char *filepath[SIFS_MAX_ENTRIES];
    int dirdepth;
    getPath(pathname, filepath, &dirdepth);

    // OPEN VOLUME
    FILE *vol = fopen(volumename, "r");

    // CHECK IF VOL IS A VOLUME
    if(isVol(volumename, vol) != 0) {
        SIFS_errno = SIFS_ENOTVOL;
        return 1;
    }

    // GET HEADER
    SIFS_VOLUME_HEADER header;
    fseek(vol, 0, SEEK_SET);
    memset(&header, 0, sizeof(header));
    fread(&header, sizeof(SIFS_VOLUME_HEADER), 1, vol);

    // GET BITMAP
    uint32_t nblocks = header.nblocks;
    SIFS_BIT bitmap[nblocks];
    memset(bitmap, 0, sizeof(bitmap));
    fread(&bitmap, sizeof(SIFS_BIT), nblocks, vol);

    // GET PARENT
    SIFS_DIRBLOCK parent_dir;
    SIFS_BLOCKID file_blockid = 0;
    int dir_entry;
    int current_depth = 1;

    // GET ROOT
    memset(&parent_dir, 0, sizeof(SIFS_DIRBLOCK));
    fread(&parent_dir, sizeof(SIFS_DIRBLOCK), 1, vol);

    if(dirdepth == 1) {
        findDir(volumename,filepath[current_depth],parent_dir,&file_blockid,&dir_entry);
    }
    else {
        while (strcmp(parent_dir.name,filepath[dirdepth -1]) != 0
                && current_depth <= dirdepth) {
            findDir(volumename,filepath[current_depth++],parent_dir,&file_blockid,&dir_entry);
            fseek(vol, FILE_POS, SEEK_SET);
            memset(&parent_dir, 0, sizeof(SIFS_DIRBLOCK));
            fread(&parent_dir, sizeof(SIFS_DIRBLOCK), 1, vol);
        }
    }

    // CHECK IF FILE
    if (bitmap[file_blockid] != SIFS_FILE) {
        SIFS_errno = SIFS_ENOTFILE;
        return 1;
    }

    // NO SUCH FILE
    if(entryExists(vol, parent_dir, filepath[dirdepth]) != 0) {
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }

    // GET FILE
    SIFS_FILEBLOCK file;
    uint32_t fileindex;

    for (int i = 0; i < parent_dir.nentries; i++) {
        file_blockid = parent_dir.entries[i].blockID;
        fileindex = parent_dir.entries[i].fileindex;

		fseek(vol, ROOT_POS + (file_blockid * header.blocksize), SEEK_SET);
		memset(&file, 0, sizeof(SIFS_FILEBLOCK));
		fread(&file, sizeof(SIFS_FILEBLOCK), 1, vol);
        if (strcmp(file.filenames[fileindex],filepath[dirdepth])==0) {
            break;
        }
    }

    // CLOSE VOL
    fclose(vol);

    // ASSIGN VALUES TO POINTERS
    *length = file.length;
    *modtime = file.modtime;

    // RETURN 0 SUCCESS!
    SIFS_errno	= SIFS_EOK;
    return 0;
}
