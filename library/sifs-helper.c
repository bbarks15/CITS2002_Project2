#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sifs-internal.h"
#include "sifs-helper.h"

int isVol(const char *volumename, FILE *vol)
{
	struct stat sbuf;
	// IF STAT IS POSSIBLE
	if(stat(volumename, &sbuf) != 0) {
		return 1;
	}
	// IF SIZE IS VALID
	else if(sbuf.st_size < sizeof(SIFS_VOLUME_HEADER)) {
		return 1;
	}
	// IF HEADER IS VALID
	else {
		SIFS_VOLUME_HEADER header;
		fread(&header, sizeof(SIFS_VOLUME_HEADER), 1, vol);
		rewind(vol);

		if(header.nblocks < 1 || header.blocksize < SIFS_MIN_BLOCKSIZE) {
			return 1;
		}
		else {
			int expected	= sizeof(SIFS_VOLUME_HEADER) +
								(header.nblocks * sizeof(SIFS_BIT)) +
								(header.nblocks * header.blocksize);
			if(sbuf.st_size != expected) {
				return 1;
			}

		}
	}
	return 0;
}

void findDir(const char *volumename, char *entry, SIFS_DIRBLOCK dir,
				SIFS_BLOCKID *blockid, int *entrynumber)
{
        // OPEN VOL
	FILE *vol = fopen(volumename,"r");

        // GET HEADER
	SIFS_VOLUME_HEADER header;
	fread(&header, sizeof(SIFS_VOLUME_HEADER), 1, vol);

        // GET BITMAP
	SIFS_BIT bitmap[header.nblocks];
	fread(&bitmap,sizeof(SIFS_BIT),header.nblocks, vol);

        // GET CHILD
	SIFS_DIRBLOCK childdir;
        SIFS_FILEBLOCK childfile;
	SIFS_BLOCKID child_block_id;
        uint32_t fileindex;

	for (int i = 0; i < dir.nentries ; i++)
	{
            // IF DIR
            if(bitmap[dir.entries[i].blockID] == SIFS_DIR) {
		child_block_id = dir.entries[i].blockID;

	        fseek(vol, ROOT_POS + (child_block_id * header.blocksize), SEEK_SET);
                memset(&childdir, 0, sizeof(SIFS_DIRBLOCK));
                fread(&childdir, sizeof(SIFS_DIRBLOCK), 1, vol);

                if (strcmp(childdir.name,entry)==0) {
			*blockid = child_block_id;
			*entrynumber = i;
			break;
                }
            }
            // IF FILE
            if(bitmap[dir.entries[i].blockID] == SIFS_FILE) {
		child_block_id = dir.entries[i].blockID;
                fileindex = dir.entries[i].fileindex;
                fseek(vol, ROOT_POS + (child_block_id * header.blocksize),
				SEEK_SET);
                memset(&childfile, 0, sizeof(SIFS_FILEBLOCK));
                fread(&childfile, sizeof(SIFS_FILEBLOCK), 1, vol);
                if (strcmp(childfile.filenames[fileindex] ,entry)==0) {
                        *blockid = child_block_id;
                        *entrynumber = i;
                        break;
                }
	    }
	}

    // CLOSE VOL
    fclose(vol);
}

// TURN PATH INTO STRING ARRAY
void getPath(const char *pathname, char *patharray[], int *dirdepth)
{
        // MAKE TEMP PATHNAME
	char *tempPathname = malloc(sizeof(SIFS_MAX_NAME_LENGTH * sizeof(char)));
	strcpy(tempPathname, pathname);

        // MAKE TOKEN
	const char s[2] = "/";
	char *token;
	token = strtok(tempPathname, s);

        // FILL FILEPATH
	*dirdepth = 0;
	patharray[0] = "\0";
	while( token != NULL ) {
		(*dirdepth)++;
		patharray[*dirdepth] = token;
		token = strtok(NULL, s);
	}
}

int entryExists(FILE *vol, SIFS_DIRBLOCK dir, char *entryname) {

        // INITILAIZE VALUES
	SIFS_DIRBLOCK 		childdir;
	SIFS_FILEBLOCK 		childfile;
	uint32_t			childindex;

        // GET HEADER
	SIFS_VOLUME_HEADER 	header;
	fseek(vol, 0, SEEK_SET);
	fread(&header, sizeof(SIFS_VOLUME_HEADER), 1, vol);

        // GET BITMAP
	SIFS_BIT bitmap[header.nblocks];
	fread(&bitmap,sizeof(SIFS_BIT),header.nblocks, vol);

	for (int i = 0; i < dir.nentries; i++) {
            // IF DIR
	    if(bitmap[dir.entries[i].blockID] == SIFS_DIR) {
	        fseek(vol, ROOT_POS + (dir.entries[i].blockID * header.blocksize),
			SEEK_SET);
                memset(&childdir, 0, sizeof(SIFS_DIRBLOCK));
                fread(&childdir, sizeof(SIFS_DIRBLOCK), 1, vol);
                if (strcmp(childdir.name, entryname) == 0) {
				return 0;
		}
	    }
            // IF FILE
            if(bitmap[dir.entries[i].blockID] == SIFS_FILE) {
			childindex = dir.entries[i].fileindex;
			fseek(vol,
				ROOT_POS + (dir.entries[i].blockID * header.blocksize),
				SEEK_SET);
			memset(&childfile, 0, sizeof(SIFS_FILEBLOCK));
			fread(&childfile, sizeof(SIFS_FILEBLOCK), 1, vol);
			if(strcmp(childfile.filenames[childindex], entryname) == 0) {
				return 0;
			}
		}
	}
    return 1;
}

char *strduplicate(const char *string) {
    //MALLOC STRING
    char *destination = malloc(strlen (string) + 1);
    // RETURN NULL IF INVALID
    if (destination == NULL) return NULL;
    // COPY STRING
    strcpy(destination, string);
    // RETURN STRING
    return destination;
}
