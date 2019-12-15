#include <stdio.h>
#include <stdlib.h>
#include "sifs.h"

//  Written by Chris.McDonald@uwa.edu.au, September 2019

//  REPORT HOW THIS PROGRAM SHOULD BE INVOKED
void usage(char *progname)
{
    fprintf(stderr, "Usage: %s volumename filepath infile\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    char	*volumename;    // filename storing the SIFS volume
	char	*filepath;		// filepath in vol
	char	*infile;		// filepath of infile
	char	*data;			// ?????
	size_t	nbytes;			// number of bytes


//	ASSIGN AND VALIDATE ARGUMENTS
    if(argcount == 4) {
		volumename = argvalue[1];
		filepath = argvalue[2];
		infile = argvalue[3];

		if(volumename == NULL || filepath == NULL || infile == NULL) {
	    	usage(argvalue[0]);
		}
    }
    else {
		usage(argvalue[0]);
		exit(EXIT_FAILURE);
    }

    // ATTEMPT TO OPEN THE FILE FOR READING
    FILE *file = fopen(infile, "r");
    // READ BYTESIZE
    fseek(file, 0, SEEK_END);
    nbytes = ftell(file);
    rewind(file);

    // ALLOCATE MEMORY AND READ
    data = malloc(nbytes);
    fseek(file, 0, SEEK_SET);
    fread(data, nbytes, 1, file);

//  ATTEMPT TO WRITE FILE TO VOLUME 
    if(SIFS_writefile(volumename, filepath, data, nbytes) != 0) {
		SIFS_perror(argvalue[0]);
		exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
