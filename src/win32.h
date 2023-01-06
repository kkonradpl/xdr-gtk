#ifndef XDR_WIN32_H_
#define XDR_WIN32_H_
#include <gtk/gtk.h>

void win32_init(void);
void win32_cleanup(void);
gboolean win32_uri(GtkWidget*, gchar*, gpointer);
gint win32_dialog_workaround(GtkDialog*);
void win32_grab_focus(GtkWindow*);
void win32_dark_titlebar(GtkWidget*);
void win32_realize(GtkWidget*, gpointer);
gchar* strsep(gchar**, gchar*);

#endif
