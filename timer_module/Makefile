
INCLUDE_DIR = -I.

SOURCES = test.c \
          mytimer.c

OBJECTS := $(notdir $(SOURCES:.c=.o))

all : $(OBJECTS)
	$(CC) $(OBJECTS) -o test -lpthread

%.o : %.c
	$(CC) -g $(CFLAGS) $(INCLUDE_DIR) -o $@ -c $<
