TARGET = y86-sim
COURSE = cs220
CC = gcc
CFLAGS  = -I $$HOME/$(COURSE)/include
LDFLAGS = -L $$HOME/$(COURSE)/lib -l cs220 -l y86

$(TARGET): main.o ysim.o
	$(CC) $(CFLAGS) $(LDFLAGS) main.o ysim.o -o $(TARGET)
	rm -f *.o *~
	export LD_LIBRARY_PATH=$$HOME/cs220/lib
	cp y86-sim extras/y86-sim
clean:
	rm -f *.o *~ y86-sim extras/y86-sim
