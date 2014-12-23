#ifndef XDR_CONNECTION_H
#define XDR_CONNECTION_H
#include <gtk/gtk.h>
#ifndef G_OS_WIN32
#define closesocket(x) close(x)
#endif

#define MODE_SERIAL 0
#define MODE_SOCKET 1

#define CONN_SUCCESS 0

#define CONN_SERIAL_FAIL_OPEN    -1
#define CONN_SERIAL_FAIL_PARM_R  -2
#define CONN_SERIAL_FAIL_PARM_W  -3
#define CONN_SERIAL_FAIL_SPEED   -4

#define CONN_SOCKET_STATE_UNDEF 100
#define CONN_SOCKET_STATE_RESOLV  1
#define CONN_SOCKET_STATE_CONN    2
#define CONN_SOCKET_STATE_AUTH    3
#define CONN_SOCKET_FAIL_RESOLV  -1
#define CONN_SOCKET_FAIL_CONN    -2
#define CONN_SOCKET_FAIL_AUTH    -3
#define CONN_SOCKET_FAIL_WRITE   -4

#define SOCKET_SALT_LEN 16
#define SOCKET_AUTH_TIMEOUT 5

typedef struct conn
{
    gchar* hostname;
    gchar* port;
    gchar* password;
    volatile gboolean canceled;
    gint socketfd;
    gint state;
} conn_t;

gint open_serial(const gchar*);
gpointer open_socket(gpointer);

conn_t* conn_new(const gchar*, const gchar* port, const gchar*);
void conn_free(conn_t*);
#endif
