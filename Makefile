#-----------------------------------------
#			MAKEFILE
#
#	Author: Sharan Juangphanich
#	Date: 24 MAy 2013
#
#-----------------------------------------

TARGET	=	Jarvis

CC		=	gcc
CFLAGS	=	-std=c99 -g -Wall
LDFLAGS	=
LIBS	=	-lportaudio

BINDIR	=	bin
INCDIR	=	include
LIBDIR	=	lib
SRCDIR	=	src

SOURCES		:=	$(wildcard $(SRCDIR)/*.c)
INCLUDES	:=	$(wildcard $(INCDIR)/*.h)
OBJECTS		:=	$(patsubst %.c, %.o, $(SOURCES))

%.o:$(SRCDIR)/%.c $(INCLUDES)
	$(CC) -c -o %@ $< $(CFLAGS)

$(BINDIR)/$(TARGET):$(OBJECTS)
	$(CC) -o $@ $< $(CFLAGS) -L$(LIBDIR) $(LIBS)

.PHONY: clean

clean:
	rm $(SRCDIR)/*.o 
