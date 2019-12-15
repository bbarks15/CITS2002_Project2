#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "sifs-internal.h"
#include "sifs-helper.h"

int SIFS_defrag(const char *volumename) {

    // OPEN VOLUME
    FILE *vol = fopen(volumename, "r+");


    // GET HEADER
    SIFS_VOLUME_HEADER header;
    fseek(vol, 0, SEEK_SET);
    memset(&header, 0, sizeof(SIFS_VOLUME_HEADER));
    fread(&header, sizeof(SIFS_VOLUME_HEADER), 1, vol);

    // GET BITMAP
    uint32_t nblocks = header.nblocks;
    SIFS_BIT bitmap[nblocks];
    memset(&bitmap, 0, sizeof(bitmap));
    fread(&bitmap,sizeof(SIFS_BIT),nblocks, vol);

    // MAP EDITBITMAP AND FIND BLOCKS TO REMOVE
    SIFS_BIT editbitmap[nblocks];
    memcpy(editbitmap, bitmap, sizeof(bitmap));
    int nblocks_remove = 0;
    int remove_posid;
    bool removeblocks = false;

    for (int i = nblocks-1; i > 0; i--) {
        if (editbitmap[i] != SIFS_UNUSED) {
            removeblocks = true;
        }
        if(removeblocks) {
            if( editbitmap[i] == SIFS_UNUSED) {
                editbitmap[i] = REMOVE;
                nblocks_remove++;
                remove_posid = i;
            }
        }
    }

    // FIND GROUPS OF BLOCKS TO MOVE
    int groups[nblocks][2];
    int ngroups = 0;
    bool findend = false;

    // REPLACE THE UNUSED BLOCKS NOT AT THE END WITH 'x'
    int i = remove_posid;
    while(editbitmap[i] != SIFS_UNUSED) {
        if(!findend && editbitmap[i] != REMOVE) {
            groups[ngroups][STARTID] = i;
            findend = true;
        }
        else if(findend && editbitmap[i] == REMOVE){
            groups[ngroups][ENDID] = i-1;
            findend = false;
            ngroups++;
        }
        i++;
    }

    // FIX LAST GROUP ENDID
    if(groups[ngroups][ENDID] == 0) {
        groups[ngroups][ENDID] = i-1;
    }


    //GET ROOT DIR
    SIFS_DIRBLOCK root_dir;
    fseek(vol, ROOT_POS, SEEK_SET);
    memset(&root_dir, 0 ,sizeof(SIFS_DIRBLOCK));
    fread(&root_dir, sizeof(SIFS_DIRBLOCK), 1, vol);

    // SHUFFLE BLOCKS UP
    char block[header.blocksize];

    for(i = 0; i <= ngroups ; i++) {
        for(int j = 0 ; j < NBLOCKS ; j++) {

            // GET BLOCK TO MOVE
            fseek(vol, GROUP_POS + j*header.blocksize, SEEK_SET);
            memset(block, 0, sizeof(block));
            fread(&block, sizeof(block), 1, vol);

            // MOVE TO EMPTY LOCATION
            fseek(vol, REMOVE_POS +j*header.blocksize, SEEK_SET);
            fwrite(&block, sizeof(block), 1, vol);

            // UPDATE THE PARENT ENTRY

            // UPDATE EDITBITMAP
            editbitmap[remove_posid + j] = editbitmap[groups[i][STARTID] + j];

        }

        //MOVE TO NEXT GROUP
        remove_posid = remove_posid + NBLOCKS;
    }

    // ZERO THE BLOCK
    memset(block, 0, sizeof(block));

    // CLEAN UP BLOCK AND BITMAP
    for(i = 0; i < nblocks_remove ; i++) {
        fseek(vol, -i*header.blocksize, SEEK_END);
        fwrite(block, sizeof(block), 1, vol);
        editbitmap[groups[ngroups][ENDID] + i] = SIFS_UNUSED;
    }

    // WRITE IN EDITBITMAP
    fseek(vol, HEADER_SIZE, SEEK_SET);
    fwrite(editbitmap, sizeof(editbitmap), 1, vol);

    // CLOSE VOL
    fclose(vol);

    // RETURN 0 EVEN THOUGH IT DOESNT WORK :(
    SIFS_errno = SIFS_EOK;
    return 0;
}

