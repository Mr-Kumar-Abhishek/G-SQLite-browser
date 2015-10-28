CC = gcc
CFLAGS = -Wall `pkg-config --cflags --libs gtk+-2.0 gtksourceview-2.0 sqlite3 gthread-2.0` -export-dynamic
OBJS = main.o

sqlitegtk: ${OBJS} glade_convert
	${CC} -o sqlitegtk ${CFLAGS} ${OBJS}

glade_convert:
	gtk-builder-convert main_window.glade main_window.xml

clean:
	rm -f sqlitegtk ${OBJS}
