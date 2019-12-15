#include <stdio.h>

#define HEADER_SIZE		sizeof(SIFS_VOLUME_HEADER)
#define BITMAP_SIZE		sizeof(bitmap)
#define ROOT_POS		HEADER_SIZE + BITMAP_SIZE
#define PARENT_POS      ROOT_POS + (parentdir_blockid * header.blocksize)
#define NEW_POS         ROOT_POS + (dir_block_id * header.blocksize)
#define FILE_POS        ROOT_POS + (file_blockid * header.blocksize)
#define TEMPNEW_POS     ROOT_POS + (temp_blockid * header.blocksize)
#define REMOVE          'x'
#define STARTID          0
#define ENDID            1
#define REMOVE_POS       ROOT_POS + (remove_posid * header.blocksize)
#define GROUP_POS        ROOT_POS + (groups[i][STARTID] * header.blocksize)
#define END_GROUP_POS    ROOT_POS + (groups[ngroups][ENDID] * header.blocksize)
#define NBLOCKS          groups[i][ENDID] - groups[i][STARTID] + 1
#define GROUPSIZE        sizeof(sizeof(block)*nblocks)

// HELPER FUNCTIONS
// Check if entry exists
extern int entryExists(FILE *vol, SIFS_DIRBLOCK dir, char *entryname);

// Not a volume
extern int isVol(const char *volumename, FILE *vol);

// Duplicate String
extern char *strduplicate(const char *string);

// Pathname Function
extern void getPath(const char *pathname, char *patharray[], int *dirdepth);

// Find a Dir
extern void findDir(const char *volumename, char *entry, SIFS_DIRBLOCK dir,
				SIFS_BLOCKID *blockid, int *entrynumber);
