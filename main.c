#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <sqlite3.h>
#include <string.h>
#include <stdio.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
GtkBuilder *builder = NULL;
GtkWidget *window = NULL;
sqlite3 *db = NULL;
GtkSourceView *srcView = NULL;
void *execute_thread(void* ptr)
{
	gchar* data = (gchar*)ptr;
	g_printf("This is from a new thread: %s\n", data);
	g_free(ptr);	
	gdk_threads_enter();
	GError *err = NULL;
	GdkPixbufAnimation *ani = gdk_pixbuf_animation_new_from_file ("loader.gif", &err);
	g_assert(err == NULL);
	gtk_image_set_from_animation(GTK_IMAGE(gtk_builder_get_object(builder, "editor_status_image")), ani);
	gdk_flush();
	gdk_threads_leave();
	g_usleep(G_USEC_PER_SEC*10);	
	gdk_threads_enter();
	gtk_image_clear(GTK_IMAGE(gtk_builder_get_object(builder, "editor_status_image")));
	gdk_flush();
	gdk_threads_leave();	
	return 0;
}
// Closes the currently active database
void close_database()
{	
	if(db)
	{
		sqlite3_close(db);
		g_printf("Closing SQLite Database...\n");
	}
}
// Executes a given SQL query
void execute_sql_query(char* query)
{
	g_assert(db != NULL);
	char **table = NULL;
	int rows = 0, cols = 0;
	char *errMsg = NULL;
	if(sqlite3_get_table(db, query, &table, &rows, &cols, &errMsg)==SQLITE_OK)
	{
		int i, j;
		for(i = 0; i <= rows; i++)
		{
			for(j = 0; j < cols; j++)
			{
				g_printf("%s", table[i*cols+j]);
			}
			g_printf("\n");
		}
		sqlite3_free_table(table);
	}
	else
	{
		printf("Error occurred\n");
		printf("%s", errMsg);
		if(errMsg)
			sqlite3_free(errMsg);
	}
}
// Opens a database using the given filename

void open_database (char *filename)
{
	close_database(); // If we already have a database open, close it first
	if(sqlite3_open(filename, &db)==SQLITE_OK)
	{
		char **table = NULL;
		int rows = 0, cols = 0;
		char *errMsg = NULL;
		if(sqlite3_get_table(db, "SELECT * FROM sqlite_master;", &table, &rows, &cols, &errMsg)==SQLITE_OK)
		{
			int i = 0;
			for(i = 0; i < (rows+1) * cols; i++)
			{
				g_printf("%s\n", table[i]);
			}
			sqlite3_free_table(table);
		}
		else
		{
			printf("Error occurred\n");
			printf("%s", errMsg);
			if(errMsg)
				sqlite3_free(errMsg);
		}
	}
	else
	{
		printf("An error occured!\n");

	}
}
/**
 * Called when a Window is destroyed
 */
void on_window_destroy (GtkObject *object, gpointer user_data)
{
	close_database();
	gtk_main_quit ();
}
void editor_toolbar_apply_clicked (GtkObject *object, gpointer user_data)
{
	g_assert(srcView != NULL);
	GtkSourceBuffer *buf = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(srcView)));
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(buf), &start, &end);
	gchar *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buf), &start, &end, FALSE);
	g_printf("%s\n", text);
	execute_sql_query(text);	
	GError *error = NULL;
	if (!g_thread_create(execute_thread, g_strdup(text), FALSE, &error))
    {
      g_printerr ("Failed to create YES thread: %s\n", error->message);
//      return 1;
    }
	g_free(text);
}
// Called on Help -> About
void menu_help_about_activate (GtkObject *object, gpointer user_data)
{
	gchar *authors[] = {"Aly Hirani", NULL};
	gtk_show_about_dialog(GTK_WINDOW(window), "authors", authors, "comments", "A simple GTK+ GUI to SQLite", 
		"copyright", "Copyright(c) 2009 Aly Hirani",
		"program-name", "SQLiteGTK+", "version", "0.0.1a", NULL);
}
// Called on File -> New
void menu_file_new_activate (GtkObject *object, gpointer user_data)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new ("Create Database", GTK_WINDOW(window), 
		GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_NEW, GTK_RESPONSE_ACCEPT, NULL);	
	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		printf("%s", filename);
		g_free(filename);
	}
	gtk_widget_destroy (dialog);
}
// Called when the Open Database menu is clicked on from the file menu
void menu_file_open_activate (GtkObject *object, gpointer user_data)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new ("Open Database", GTK_WINDOW(window), 
	GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);	
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.db");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		open_database(filename);
		g_free (filename);
	}
	// Is filter not required to be free'd?
	gtk_widget_destroy (dialog);
}
// Creates a GTKSourceView
GtkSourceView *create_gtk_source_view()
{
	GtkSourceView *view;
	GtkSourceBuffer *buf;
	GtkSourceLanguage *lang;
	GtkSourceLanguageManager *lm;
	PangoFontDescription *font_desc;
	lm = gtk_source_language_manager_new();
	buf = gtk_source_buffer_new(NULL);
	g_object_ref(lm);
	g_object_set_data_full(G_OBJECT(buf), "language-manager", lm, (GDestroyNotify)g_object_unref);
	lang = gtk_source_language_manager_guess_language(lm, NULL, "text/x-sql");
	if(lang != NULL)
	{
		gtk_source_buffer_set_language(buf, lang);
		gtk_source_buffer_set_highlight_syntax(buf, TRUE);
	}
	view = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(buf));
	g_object_set (G_OBJECT (view), "auto-indent", TRUE, "highlight-current-line", TRUE, "indent-on-tab", 
		TRUE, "tab-width", 4, "show-line-numbers", TRUE, NULL);
	font_desc = pango_font_description_from_string ("mono 12");
	gtk_widget_modify_font (GTK_WIDGET(view), font_desc);
	pango_font_description_free (font_desc);
	return view;
}

// Create a new page in the notebook, with the GtkSourceView appended, with the given caption
void sqlitegtk_create_notebook_page(GtkNotebook *notebook, gchar *caption)

{
	g_assert(notebook != NULL);	
	GtkWidget *scrollWindow = gtk_scrolled_window_new(NULL, NULL);
	g_assert(scrollWindow != NULL);		
	// TODO: Remove srcView dependencies
	srcView = create_gtk_source_view();
	g_assert(srcView != NULL);	
	gtk_container_add (GTK_CONTAINER(scrollWindow), GTK_WIDGET(srcView));
	gint result = gtk_notebook_append_page(notebook, scrollWindow, gtk_label_new(caption));
	g_assert(result != -1);
	gtk_widget_show_all(scrollWindow);
}
// The main function 
int main (int argc, char *argv[])
{
	g_thread_init(NULL);
	gdk_threads_init();		
	gchar *db_location = NULL, *sql_location = NULL;
	GOptionEntry entries[] = {
		{ "database", 'd', 0, G_OPTION_ARG_STRING, &db_location, "Database", "D" },
		{ "sql", 's', 0, G_OPTION_ARG_STRING, &sql_location, "SQL Queries", "S" },
		{ NULL } };
	GError *error = NULL;
	GOptionContext *context;
	context = g_option_context_new ("some stupid piece of software");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (!g_option_context_parse (context, &argc, &argv, &error))
	{
		g_print ("option parsing failed: %s\n", error->message);
		return 1;
    }
    if(db_location != NULL)
    {
		open_database(db_location);
		g_free(db_location);
	}
    if(sql_location != NULL)
    {
	    g_printf("Not yet supported: %s\n", sql_location);
	    g_free(sql_location);
	}
	gtk_init (&argc, &argv);
	builder = gtk_builder_new ();
	g_assert(builder != NULL);	
	gtk_builder_add_from_file (builder, "main_window.glade", NULL);
	gtk_builder_connect_signals (builder, NULL);
	GtkNotebook *notebook = GTK_NOTEBOOK(gtk_builder_get_object(builder, "editor_notebook"));
	sqlitegtk_create_notebook_page(notebook, "My Awesome Page 1");
	sqlitegtk_create_notebook_page(notebook, "My Awesome Page 2");
	sqlitegtk_create_notebook_page(notebook, "My Awesome Page 3");
	sqlitegtk_create_notebook_page(notebook, "My Awesome Page 4");
/*	GtkWidget *scrollWindow = GTK_WIDGET(gtk_builder_get_object(builder, "editor_scroll_window"));
	srcView = create_gtk_source_view();
	g_assert(scrollWindow != NULL);
	g_assert(srcView != NULL);	
	gtk_container_add (GTK_CONTAINER(scrollWindow), GTK_WIDGET(srcView));
	gtk_widget_show_all (scrollWindow);
*/

	window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
	g_assert(window != NULL);
	gtk_widget_show (window);
	gtk_main ();
	g_object_unref (G_OBJECT (builder));
	return 0;
}
/*	// BEGIN TEMP CODE
+	GError *err = NULL;
+	GdkPixbufAnimation *ani = gdk_pixbuf_animation_new_from_file ("/home/aly/Desktop/ajax-loader.gif", &err);
+	gtk_image_set_from_animation(GTK_IMAGE(gtk_builder_get_object(builder, "editor_loading_image")), ani);
+	// END TEMP CODE */
