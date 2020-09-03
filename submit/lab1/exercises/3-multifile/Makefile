PROJECT = quadr
SRC_FILES = main.c \
	   quadr.c \
	   quadr.h \
	   README \
	   Makefile \
	   test.data

nCFLAGS = -g -Wall -std=c11
LDFLAGS = -lm

$(PROJECT):	quadr.o main.o
		$(CC) $(LDFLAGS) quadr.o main.o -o $@

clean:
		rm -f *.o $(PROJECT) $(PROJECT).tar.gz
depend:
		$(CC) -MM $(CPPFLAGS) *.c
main.o: main.c quadr.h
quadr.o: quadr.c quadr.h

dist:	$(PROJECT).tar.gz
$(PROJECT).tar.gz:	$(SRC_FILES)
			tar -cf $(PROJECT).tar $(SRC_FILES)
			gzip -f $(PROJECT).tar
