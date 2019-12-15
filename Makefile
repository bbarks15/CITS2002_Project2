PROJECT		= sifs

HEADER		= $(PROJECT).h
LIBRARY		= lib$(PROJECT).a

APPLICATIONS	= sifs_mkvolume sifs_dirinfo sifs_mkdir sifs_writefile sifs_readfile sifs_rmfile

# ----------------------------------------------------------------

CC      = cc
CFLAGS  = -std=c99 -Wall -Werror -pedantic -g
LIBS	= -L. -lsifs -lm


all:	$(APPLICATIONS)

$(LIBRARY):
	make -C library

%:	%.c $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)


clean:
	rm -f $(LIBRARY) $(APPLICATIONS)
	make -C library clean

