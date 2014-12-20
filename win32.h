#ifndef XDR_WIN32_H_
#define XDR_WIN32_H_
#include <gtk/gtk.h>
#define WIN32_FONT_FILE "VeraMono.ttf"

void win32_init();
void win32_cleanup();
char *strsep(char**, char*);
gboolean win32_uri(GtkAboutDialog*, gchar*, gpointer);
gint win32_dialog_workaround(GtkDialog*);

#endif
