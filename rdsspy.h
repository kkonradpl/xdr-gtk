#ifndef XDR_RDSSPY_H_
#define XDR_RDSSPY_H_

void rdsspy_toggle();
gboolean rdsspy_is_up();
gboolean rdsspy_is_connected();
void rdsspy_stop();
gboolean rdsspy_init(gint);
gpointer rdsspy_server(gpointer);
gboolean rdsspy_toggle_button(gpointer);
void rdsspy_reset();
void rdsspy_send(gint, gchar*, guint);

#endif

