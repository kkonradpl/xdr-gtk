#ifndef XDR_UI_CONNECT_H
#define XDR_UI_CONNECT_H
#include <gtk/gtk.h>

void connection_toggle();
void connection_dialog(gboolean);
gboolean connection_socket_callback(gpointer);
gboolean connection_socket_callback_info(gpointer);
void connection_socket_auth_fail();

#endif

