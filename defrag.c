#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "library/sifs-internal.h"
#include "library/sifs-helper.h"

int SIFS_defrag(const char *volumename) {
    // open volume

    FILE *vol = fopen(volumename, "r+");

    // get header and bitmap

    SIFS_VOLUME_HEADER header;
    fseek(vol, 0, SEEK_SET);
    memset(&header, 0, sizeof(SIFS_VOLUME_HEADER));
    fread(&header, sizeof(SIFS_VOLUME_HEADER), 1, vol);

    uint32_t nblocks = header.nblocks;
    SIFS_BIT bitmap[nblocks];
    memset(&bitmap, 0, sizeof(bitmap));
    fread(&bitmap,sizeof(SIFS_BIT),nblocks, vol);

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

    printf(" bitmap = %s \n", bitmap);
    printf(" bitmap = %s \n", editbitmap);

    int groups[nblocks][2];
    int ngroups = 0;
    bool findend = false;

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
        printf("startid %i \n", groups[i][STARTID]);
        printf("endid %i \n", groups[i][ENDID]);
        i++;
    }

    if(groups[ngroups][ENDID] == 0) {
        groups[ngroups][ENDID] = i-1;
    }

    for(i = 0; i < ngroups + 1 ; i++) {
        printf("-----------GROUPS-------------- \n");
        printf("startid %i \n", groups[i][STARTID]);
        printf("endid %i \n", groups[i][ENDID]);
    }
    printf("remove pos %i \n", remove_posid );

    // shuffle blocks

    //root dir
    SIFS_DIRBLOCK root_dir;
    fseek(vol, ROOT_POS, SEEK_SET);
    memset(&root_dir, 0 ,sizeof(SIFS_DIRBLOCK));
    fread(&root_dir, sizeof(SIFS_DIRBLOCK), 1, vol);

    char block[header.blocksize];

    for(i = 0; i <= ngroups ; i++) {
        for(int j = 0 ; j < NBLOCKS ; j++) {

            // get block to move
            fseek(vol, GROUP_POS + j*header.blocksize, SEEK_SET);
            memset(block, 0, sizeof(block));
            fread(&block, sizeof(block), 1, vol);

            // move block to the empty location
            fseek(vol, REMOVE_POS +j*header.blocksize, SEEK_SET);
            fwrite(&block, sizeof(block), 1, vol);

            // update the parent directory entry

            // update bitmap
            editbitmap[remove_posid + j] = editbitmap[groups[i][STARTID] + j];

            //          dddxdxdddd end = 4 rm = 3
            //          dddduxdddd
            //          dddd
        }

        //move to the next group of block
        remove_posid = remove_posid + NBLOCKS;
    }

    memset(block, 0, sizeof(block));

    // clean up blocks at end
    // clean up bitmap
    for(i = 0; i < nblocks_remove ; i++) {
        fseek(vol, -i*header.blocksize, SEEK_END);
        fwrite(block, sizeof(block), 1, vol);
        editbitmap[groups[ngroups][ENDID] + i] = SIFS_UNUSED;
    }

    fseek(vol, HEADER_SIZE, SEEK_SET);
    fwrite(editbitmap, sizeof(editbitmap), 1, vol);

    fclose(vol);

    SIFS_errno = SIFS_EOK;
    return 0;
}


int main(int argc, char *argv[]) {
    SIFS_defrag(argv[1]);
    return 0;
}
