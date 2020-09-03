PROJECT = key-value
SRC_FILES = main.c \
	   key-value.c \
	   key-value.h \
	   process-file.c \
	   process-file.h \
	   README \
	   Makefile \
	   test.data

nCFLAGS = -g -Wall -std=c11
LDFLAGS = -lm

$(PROJECT):	key-value.o main.o process-file.o
		$(CC) $(LDFLAGS) key-value.o main.o process-file.o -o $@

clean:
		rm -f *.o $(PROJECT) $(PROJECT).tar.gz
depend:
		$(CC) -MM $(CPPFLAGS) *.c

key-value.o: key-value.c key-value.h
main.o: main.c process-file.h
process-file.o:	process-file.c key-value.h process-file.h

dist:	$(PROJECT).tar.gz
$(PROJECT).tar.gz:	$(SRC_FILES)
			tar -cf $(PROJECT).tar $(SRC_FILES)
			gzip -f $(PROJECT).tar
