hxcmodplayer:pcmsine.o
	$(CC) $(LDFLAGS) -lasound pcmsine.o -o hxcmodplayer
pcmsine.o:pcmsine.c
	$(CC) $(CCFLAGS) -c pcmsine.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o hxcmodplayer