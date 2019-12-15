#include <stdio.h>
#include <stdlib.h>
#include "sifs.h"

//  REPORT HOW THIS PROGRAM SHOULD BE INVOKED
void usage(char *progname) {
    fprintf(stderr, "Usage: %s volumename pathname\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[]) {

    char    *volumename;    // filename storing the SIFS volume
    char    *pathname;        // pathname of dir


//    ASSIGN AND VALIDATE ARGUMENTS
    if(argcount == 3) {
        volumename = argvalue[1];
        pathname = argvalue[2];

        if(volumename == NULL || pathname == NULL) {
            usage(argvalue[0]);
        }
    }
    else {
        usage(argvalue[0]);
        exit(EXIT_FAILURE);
    }

//  ATTEMPT TO WRITE FILE TO VOLUME
    if(SIFS_mkdir(volumename, pathname) != 0) {
        SIFS_perror(argvalue[0]);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
