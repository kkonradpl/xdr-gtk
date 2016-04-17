#ifndef XDR_WIN32_H_
#define XDR_WIN32_H_
#include <gtk/gtk.h>

void win32_init();
void win32_cleanup();
gboolean win32_uri(GtkWidget*, gchar*, gpointer);
gint win32_dialog_workaround(GtkDialog*);
void win32_grab_focus(GtkWindow*);
gchar* strsep(gchar**, gchar*);

#endif
