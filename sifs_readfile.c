#include <stdio.h>
#include <stdlib.h>
#include "sifs.h"

//  Written by Chris.McDonald@uwa.edu.au, September 2019

//  REPORT HOW THIS PROGRAM SHOULD BE INVOKED
void usage(char *progname)
{
    fprintf(stderr, "Usage: %s volumename filepath\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    char	*volumename;    // filename storing the SIFS volume
	char	*filepath;		// filepath in vol
	void	*data;			// ?????
	size_t	nbytes;			// number of bytes


//	ASSIGN AND VALIDATE ARGUMENTS
    if(argcount == 3) {
		volumename = argvalue[1];
		filepath = argvalue[2];

		if(volumename == NULL || filepath == NULL) {
	    	usage(argvalue[0]);
		}
    }
    else {
		usage(argvalue[0]);
		exit(EXIT_FAILURE);
    }

//  ATTEMPT TO WRITE FILE TO VOLUME 
    if(SIFS_readfile(volumename, filepath, &data, &nbytes) != 0) {
		SIFS_perror(argvalue[0]);
		exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
