rkpinctrl:iomux.o
	$(CC) $(LDFLAGS) iomux.o -o rkpinctrl
iomux.o:iomux.c
	$(CC) $(CCFLAGS) -c iomux.c


# remove object files and executable when user executes "make clean"
clean:
	rm *.o rkpinctrl
