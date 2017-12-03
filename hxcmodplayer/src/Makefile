hxcmodplayer:hxcmodplayer.o hxcmod.o
	$(CC) $(LDFLAGS) -lasound hxcmodplayer.o hxcmod.o -o hxcmodplayer
hxcmod.o:hxcmod.c
	$(CC) $(CCFLAGS) -c hxcmod.c
hxcmodplayer.o:hxcmodplayer.c
	$(CC) $(CCFLAGS) -c hxcmodplayer.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o hxcmodplayer