#ifndef XDR_CLIPBOARD_H_
#define XDR_CLIPBOARD_H_

gboolean clipboard_full(GtkWidget*, GdkEvent*, gpointer);
gboolean clipboard_pi(GtkWidget*, GdkEvent*, gpointer);
gboolean clipboard_ps(GtkWidget*, GdkEventButton*, gpointer);
gboolean clipboard_rt(GtkWidget*, GdkEvent*, gpointer);
#endif
