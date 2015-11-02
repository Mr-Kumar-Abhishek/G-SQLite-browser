CC = gcc
CFLAGS = -Wall `pkg-config --cflags --libs gtk+-2.0 gtksourceview-2.0 sqlite3 gthread-2.0` -export-dynamic
OBJS = main.o

sqlitegtk: ${OBJS}
	${CC} -o sqlitegtk ${CFLAGS} ${OBJS}

clean:
	rm -f sqlitegtk ${OBJS}
