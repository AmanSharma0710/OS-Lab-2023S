SHELL = /bin/sh
CC = g++
CFLAGS = -Wall
LIBNAME = libgoodmalloc.a
LIBRARY_PATH = /usr/include/lib
OBJS = goodmalloc.o
HEADERS = goodmalloc.h
FILE = goodmalloc.cpp
HEADER_PATH = /usr/include
RM = rm -f

mergesort: mergesort.cpp $(LIBNAME)
	$(CC) $(CFLAGS) -o $@ $^

$(LIBNAME): $(OBJS)
	$(AR) rcs $@ $^

OBJS: $(FILE)
	$(CC) $(CFLAGS) -c $^

install: $(LIBNAME)
	mkdir -p $(LIBRARY_PATH)
	cp $(LIBNAME) $(LIBRARY_PATH)/

headers: $(HEADERS)
	cp $(HEADERS) $(HEADER_PATH)

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) $(LIBNAME)