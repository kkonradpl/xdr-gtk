#ifndef XDR_RDSSPY_H_
#define XDR_RDSSPY_H_

void rdsspy_toggle();
gboolean rdsspy_is_up();
gboolean rdsspy_is_connected();
void rdsspy_stop();

void rdsspy_reset();
void rdsspy_send(gint, gchar*, guint);

#endif

