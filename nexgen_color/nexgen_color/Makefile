# Makefile for gen2
# Copyright(c) 2003, 2004 by Kyongsu Son, Photon Dream.
# All Rights Reserved.

CC = gcc
FLAGS = -O2 -Wall
LIBS = -lm -I/usr/src/linux/include -lusb
LIBPATH = 

NEXGENOBJECTS = nexgen.o 

all: nexgen 

nexgen: $(NEXGENOBJECTS)
	$(CC) $(FLAGS) -o nexgen $(NEXGENOBJECTS) $(LIBS) $(LIBPATH)

nexgen.o: nexgen.h nexgen.c 
	$(CC) $(FLAGS) -c nexgen.c

.PHONY: clean

clean: 
	-rm -f $(NEXGENOBJECTS) nexgen  

#EOF
