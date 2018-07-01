EXECUTABLE = rw_gan_mario_test

OBJS       = rw_gan_mario_test.o

CC         = gcc
CFLAGS     = -g $(shell python3-config --include) -I$(shell pwd)

LINKER     = gcc
LDFLAGS    = -L/Users/tea/anaconda3/lib -L$(shell pwd) $(shell python3-config --ldflags) -lgenmariolib

.PHONY: clean cython

all: cython $(EXECUTABLE)

cython:
	python setup.py build_ext --inplace

.c.o:
	$(CC) $(CFLAGS) -c $<

$(EXECUTABLE) : $(OBJS)
	$(LINKER) -o $(EXECUTABLE) $(OBJS) $(LDFLAGS)

clean:
	rm -rf *.o *.so ganmariolib.c ganmariolib.h core build $(EXECUTABLE)
