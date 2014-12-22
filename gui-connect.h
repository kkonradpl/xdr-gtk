#ifndef XDR_GUI_CONNECT_H
#define XDR_GUI_CONNECT_H
#include <gtk/gtk.h>

void connection_toggle();
void connection_dialog(gboolean);
void connection_dialog_destroy(GtkWidget*, gpointer);
void connection_dialog_cancel(GtkWidget*, gpointer);
void connection_dialog_select(GtkWidget*, gpointer);
void connection_dialog_connect(GtkWidget*, gpointer);
void connection_dialog_status(const gchar*);
void connection_dialog_unlock(gboolean);
gboolean connection_dialog_key(GtkWidget*, GdkEventKey*, gpointer);
void connection_serial_state(gint);
gboolean connection_dialog_socket_state(gpointer);
void connection_dialog_connected(gint);
gboolean connection_socket_callback(gpointer);
gboolean connection_socket_callback_info(gpointer);
gboolean connection_socket_auth_fail(gpointer);

#endif

