#ifndef XDR_CLIPBOARD_H_
#define XDR_CLIPBOARD_H_

gboolean clipboard_full(GtkWidget*, GdkEvent*, gpointer);
gboolean clipboard_pi(GtkWidget*, GdkEvent*, gpointer);
gboolean clipboard_str(GtkWidget*, GdkEvent*, gpointer);
gchar* replace_spaces(gchar*);
gchar* get_pi();
#endif
