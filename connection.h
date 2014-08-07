#ifndef XDR_CONNECTION_H
#define XDR_CONNECTION_H
#ifdef G_OS_WIN32
#include <windows.h>
#else
#define closesocket(x) close(x)
#endif

#define CONN_SERIAL 1
#define CONN_SOCKET 2
#define SOCKET_SALT 16

void connection_dialog();
gboolean connection();
void connection_cancel(GtkWidget*, gpointer);
gpointer read_thread(gpointer);
void read_parse(gchar, gchar[]);
int open_serial(const gchar*);
int open_socket(const gchar*, guint, const gchar*);

#endif
