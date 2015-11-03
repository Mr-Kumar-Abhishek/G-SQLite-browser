CC = gcc
CFLAGS := $(shell pkg-config --cflags glib-2.0 gtk+-2.0 gmodule-2.0 gtksourceview-2.0 gthread-2.0)
LDFLAGS := $(shell pkg-config --libs glib-2.0 gtk+-2.0 gmodule-2.0 gtksourceview-2.0 gthread-2.0)
LIBS := -lsqlite3
DEBUG = -Wall -Werror
OBJS = main.o

sqlitegtk: ${OBJS}
	$(CC) $(DEBUG) $(CFLAGS) $< -o $@ ${LDFLAGS} $(LIBS) 

clean:
	rm -f sqlitegtk ${OBJS}
