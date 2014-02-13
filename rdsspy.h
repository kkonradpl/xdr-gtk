#ifndef XDR_RDSSPY_H_
#define XDR_RDSSPY_H_

#define RDSSPY_READ_BUFF 100

void rdsspy_toggle();
gboolean rdsspy_is_up();
gboolean rdsspy_is_connected();
void rdsspy_stop();
void rdsspy_init(int);
gpointer rdsspy_server(gpointer);
gboolean rdsspy_checkbox_disable(gpointer);
void rdsspy_reset();
void rdsspy_send(gint, gchar*, guint);

extern gint rdsspy_socket;
extern gint rdsspy_client;

#endif

