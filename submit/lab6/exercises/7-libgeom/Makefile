CFLAGS = -g -Wall -fPIC -std=c11
OBJS = \
	circ.o \
	rect.o \
	testgeom.c 

all: static dynamic testgeom-static testgeom-dynamic


static:	$(OBJS)
	ar rcs $@ $(OBJS)

dynamic: 	$(OBJS)
		$(CC) -shared $(OBJS)

testgeom-static:	$(OBJS)
			$(CC) $(OBJS) -static -L -lvec -o testgeom-static

testgeom-dynamic: 	$(OBJS)
			$(CC) $(OBJS) -L -lvec -o testgeom-dynamic

